#include "ui.h"
#include "errorlog.h"
#include "shader.h"
#include "vecmath.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================
 *  Thin stroke font
 *  Each glyph is a set of polylines on a grid where the baseline is y = 0,
 *  the x-height is 5, the capital height is 7 and descenders reach -2.
 *  Strokes are separated by '|', points are "x,y". The renderer draws every
 *  segment as an anti-aliased capsule, which gives a light, even, modern look.
 * ======================================================================== */
typedef struct { int ch; float adv; const char *strokes; } GlyphDef;
static const GlyphDef FONT_DEF[] = {
    /* capitals */
    {'A', 7.5f, "0,0 3,7 6,0|1,2.4 5,2.4"},
    {'B', 7.0f, "0,0 0,7 3.2,7 4.5,6.2 4.5,4.8 3.2,3.6 0,3.6|3.2,3.6 4.8,2.9 5,1.4 4,0.3 3,0 0,0"},
    {'C', 7.5f, "5.2,5.8 4.2,6.8 2.7,7 1.3,6.4 0.4,5 0,3.5 0.4,2 1.3,0.6 2.7,0 4.2,0.2 5.2,1.2"},
    {'D', 7.5f, "0,0 0,7 2.5,7 4.3,6.2 5.3,4.7 5.5,3.5 5.3,2.3 4.3,0.8 2.5,0 0,0"},
    {'E', 6.5f, "4.6,7 0,7 0,0 4.6,0|0,3.6 3.6,3.6"},
    {'F', 6.5f, "4.6,7 0,7 0,0|0,3.6 3.6,3.6"},
    {'G', 7.8f, "5.2,5.8 4.2,6.8 2.7,7 1.3,6.4 0.4,5 0,3.5 0.4,2 1.3,0.6 2.7,0 4.2,0.2 5.4,1.2 5.6,3 3.2,3"},
    {'H', 7.2f, "0,0 0,7|5,0 5,7|0,3.6 5,3.6"},
    {'I', 2.8f, "0,0 0,7"},
    {'J', 6.0f, "4,7 4,1.8 3.4,0.5 2,0 0.8,0.4 0.2,1.5"},
    {'K', 6.8f, "0,0 0,7|5,7 0,2.8|1.6,3.9 5,0"},
    {'L', 6.0f, "0,7 0,0 4.5,0"},
    {'M', 8.4f, "0,0 0,7 3,2.4 6,7 6,0"},
    {'N', 7.2f, "0,0 0,7 5,0 5,7"},
    {'O', 7.8f, "2.7,7 1.3,6.6 0.4,5.3 0,3.5 0.4,1.7 1.3,0.4 2.7,0 4.1,0.4 5,1.7 5.4,3.5 5,5.3 4.1,6.6 2.7,7"},
    {'P', 6.6f, "0,0 0,7 3,7 4.5,6.4 4.8,5.2 4.3,4 3,3.4 0,3.4"},
    {'Q', 7.8f, "2.7,7 1.3,6.6 0.4,5.3 0,3.5 0.4,1.7 1.3,0.4 2.7,0 4.1,0.4 5,1.7 5.4,3.5 5,5.3 4.1,6.6 2.7,7|3.3,1.8 5.6,-0.6"},
    {'R', 7.0f, "0,0 0,7 3,7 4.5,6.4 4.8,5.2 4.3,4 3,3.4 0,3.4|2.6,3.4 5,0"},
    {'S', 6.6f, "4.8,5.8 3.8,6.8 2.4,7 1,6.6 0.3,5.4 0.6,4.2 2.5,3.5 4.3,2.8 4.9,1.6 4.5,0.5 3.3,0 1.7,0.1 0.4,1 0,2"},
    {'T', 6.6f, "0,7 5,7|2.5,7 2.5,0"},
    {'U', 7.2f, "0,7 0,2.2 0.5,0.8 1.5,0.1 2.5,0 3.5,0.1 4.5,0.8 5,2.2 5,7"},
    {'V', 7.2f, "0,7 2.75,0 5.5,7"},
    {'W', 9.6f, "0,7 2,0 4,4.8 6,0 8,7"},
    {'X', 7.2f, "0,7 5.5,0|5.5,7 0,0"},
    {'Y', 7.2f, "0,7 2.75,3.5 5.5,7|2.75,3.5 2.75,0"},
    {'Z', 6.6f, "0,7 5,7 0,0 5,0"},
    /* lowercase */
    {'a', 6.0f, "4,5 4,0|4,2.5 3.8,3.9 3,4.8 2,5 1,4.7 0.3,3.8 0,2.5 0.3,1.2 1,0.3 2,0 3,0.2 3.8,1.1 4,2.5"},
    {'b', 6.0f, "0,7.2 0,0|0,2.5 0.2,3.9 1,4.7 2,5 3,4.7 3.7,3.8 4,2.5 3.7,1.2 3,0.3 2,0 1,0.3 0.2,1.1 0,2.5"},
    {'c', 5.4f, "3.8,4 3,4.8 2,5 1,4.7 0.3,3.8 0,2.5 0.3,1.2 1,0.3 2,0 3,0.2 3.8,1"},
    {'d', 6.0f, "4,7.2 4,0|4,2.5 3.8,3.9 3,4.7 2,5 1,4.7 0.3,3.8 0,2.5 0.3,1.2 1,0.3 2,0 3,0.3 3.8,1.1 4,2.5"},
    {'e', 6.0f, "0,2.5 4,2.5 3.9,3.8 3.2,4.7 2,5 0.9,4.7 0.2,3.8 0,2.5 0.2,1.2 0.9,0.3 2,0 3.1,0.2 3.8,0.9"},
    {'f', 4.2f, "3.2,6.9 2.4,7.2 1.6,6.9 1.3,6 1.3,0|0,5 3,5"},
    {'g', 6.0f, "4,5 4,-0.8 3.6,-1.7 2.6,-2.1 1.5,-2 0.7,-1.5|4,2.5 3.8,3.9 3,4.8 2,5 1,4.7 0.3,3.8 0,2.5 0.3,1.2 1,0.3 2,0 3,0.2 3.8,1.1 4,2.5"},
    {'h', 6.0f, "0,7.2 0,0|0,3.2 0.6,4.4 1.6,5 2.6,4.9 3.4,4.2 3.7,3 3.7,0"},
    {'i', 2.8f, "0,0 0,5|0,6.4 0,6.5"},
    {'j', 3.6f, "1.7,5 1.7,-1 1.4,-1.8 0.6,-2.1 0,-2|1.7,6.4 1.7,6.5"},
    {'k', 5.6f, "0,7.2 0,0|3.8,5 0,1.8|1.2,2.9 4,0"},
    {'l', 2.8f, "0,7.2 0,0"},
    {'m', 8.0f, "0,0 0,5|0,3.4 0.5,4.5 1.4,5 2.3,4.8 2.9,3.9 2.9,0|2.9,3.9 3.5,4.6 4.4,5 5.3,4.7 5.8,3.8 5.8,0"},
    {'n', 6.0f, "0,0 0,5|0,3.2 0.6,4.4 1.6,5 2.6,4.9 3.4,4.2 3.7,3 3.7,0"},
    {'o', 6.0f, "2,5 1,4.7 0.3,3.8 0,2.5 0.3,1.2 1,0.3 2,0 3,0.3 3.7,1.2 4,2.5 3.7,3.8 3,4.7 2,5"},
    {'p', 6.0f, "0,5 0,-2|0,2.5 0.2,3.9 1,4.7 2,5 3,4.7 3.7,3.8 4,2.5 3.7,1.2 3,0.3 2,0 1,0.3 0.2,1.1 0,2.5"},
    {'q', 6.0f, "4,5 4,-2|4,2.5 3.8,3.9 3,4.7 2,5 1,4.7 0.3,3.8 0,2.5 0.3,1.2 1,0.3 2,0 3,0.3 3.8,1.1 4,2.5"},
    {'r', 4.2f, "0,0 0,5|0,3.2 0.6,4.4 1.6,5 2.6,5"},
    {'s', 5.0f, "3.4,4.2 2.8,4.8 1.8,5 0.8,4.8 0.3,4 0.6,3.1 1.7,2.6 2.9,2.1 3.4,1.3 3,0.4 2,0 1,0.2 0.2,0.9"},
    {'t', 4.4f, "1.3,6.5 1.3,1 1.6,0.2 2.4,0 3,0.2|0,5 3,5"},
    {'u', 6.0f, "0,5 0,2 0.3,0.8 1,0.2 2,0 2.9,0.3 3.7,1.3|3.7,5 3.7,0"},
    {'v', 5.6f, "0,5 2,0 4,5"},
    {'w', 7.6f, "0,5 1.5,0 3,4 4.5,0 6,5"},
    {'x', 5.6f, "0,5 4,0|4,5 0,0"},
    {'y', 5.6f, "0,5 2,0|4,5 1.3,-2"},
    {'z', 5.6f, "0,5 4,5 0,0 4,0"},
    /* digits */
    {'0', 6.6f, "2.2,7 1.1,6.6 0.4,5.4 0.1,3.5 0.4,1.6 1.1,0.4 2.2,0 3.3,0.4 4,1.6 4.3,3.5 4,5.4 3.3,6.6 2.2,7"},
    {'1', 5.0f, "0.8,5.6 2.4,7 2.4,0"},
    {'2', 6.6f, "0.2,5.6 0.6,6.5 1.5,7 2.6,7 3.6,6.5 4.1,5.5 3.8,4.3 3,3.3 0,0 4.4,0"},
    {'3', 6.6f, "0.3,5.9 1.2,6.8 2.4,7 3.5,6.6 4,5.6 3.6,4.5 2.4,3.8 1.5,3.7|2.4,3.8 3.8,3.2 4.3,2 3.9,0.7 2.9,0.1 1.6,0 0.6,0.5 0.1,1.4"},
    {'4', 6.6f, "3.6,0 3.6,7 0,2 4.6,2"},
    {'5', 6.6f, "4.2,7 0.7,7 0.4,3.8|0.4,3.8 1.3,4.3 2.4,4.4 3.6,4 4.2,3 4.3,1.8 3.8,0.7 2.8,0.1 1.6,0 0.6,0.4 0.1,1.3"},
    {'6', 6.6f, "3.8,6.2 3,6.9 2,7 1,6.5 0.4,5.3 0.1,3.5 0.2,1.7 0.8,0.5 1.8,0 2.8,0.1 3.7,0.8 4.2,2 3.9,3.2 3,3.9 1.9,4 0.9,3.6 0.3,2.8"},
    {'7', 6.6f, "0,7 4.4,7 1.6,0"},
    {'8', 6.6f, "2.2,3.7 1.1,4 0.5,4.9 0.7,6 1.4,6.8 2.2,7 3,6.8 3.7,6 3.9,4.9 3.3,4 2.2,3.7 0.9,3.4 0.2,2.4 0,1.5 0.4,0.5 1.2,0.1 2.2,0 3.2,0.1 4,0.5 4.3,1.5 4,2.5 3.4,3.4 2.2,3.7"},
    {'9', 6.6f, "0.5,0.8 1.3,0.1 2.3,0 3.3,0.5 3.9,1.7 4.2,3.5 4.1,5.3 3.5,6.5 2.5,7 1.5,6.9 0.6,6.2 0.1,5 0.4,3.8 1.3,3.1 2.4,3 3.4,3.4 4,4.2"},
    /* punctuation */
    {' ', 3.4f, ""},
    {'.', 2.6f, "0,0 0,0.1"},
    {',', 2.6f, "0.4,0.2 0,-1.3"},
    {':', 2.6f, "0,0 0,0.1|0,4.2 0,4.3"},
    {';', 2.6f, "0,4.2 0,4.3|0.4,0.2 0,-1.3"},
    {'-', 4.4f, "0,3.2 3,3.2"},
    {'+', 6.0f, "0,3.2 4,3.2|2,1.2 2,5.2"},
    {'/', 4.6f, "0,-0.5 3,7.2"},
    {'(', 4.0f, "2,7.5 0.8,5.8 0.4,3.5 0.8,1.2 2,-0.5"},
    {')', 4.0f, "0,7.5 1.2,5.8 1.6,3.5 1.2,1.2 0,-0.5"},
    {'%', 8.0f, "5,7 0,0|1,7 0.3,6.7 0,5.8 0.3,5 1,4.7 1.8,5 2.1,5.8 1.8,6.7 1,7|4,2.3 3.3,2 3,1.1 3.3,0.3 4,0 4.8,0.3 5.1,1.1 4.8,2 4,2.3"},
    {'\'', 2.4f, "0,7 0,5.8"},
    {'"', 3.6f, "0,7 0,5.8|1.6,7 1.6,5.8"},
    {'?', 5.8f, "0.3,5.8 0.9,6.8 2,7 3.1,6.7 3.6,5.7 3.2,4.6 2,3.8 1.8,2.8|1.8,0.1 1.8,0"},
    {'!', 2.8f, "0,7 0,2|0,0.1 0,0"},
    {'=', 6.0f, "0,2.4 4,2.4|0,4 4,4"},
    {'>', 5.6f, "0,5 3.5,3 0,1"},
    {'<', 5.6f, "3.5,5 0,3 3.5,1"},
    {'_', 5.6f, "0,-0.8 5,-0.8"},
    {'*', 4.6f, "0,3.5 0,3.6"},
};
#define FONT_COUNT ((int)(sizeof FONT_DEF / sizeof FONT_DEF[0]))
#define MAX_SEGS 3600

typedef struct { float adv; int first, count; } Glyph;
static Glyph g_glyph[128];
static float g_seg[MAX_SEGS][4];
static int g_nseg = 0;

static void build_font(void) {
    g_nseg = 0;
    memset(g_glyph, 0, sizeof g_glyph);
    for (int i = 0; i < FONT_COUNT; ++i) {
        Glyph *g = &g_glyph[FONT_DEF[i].ch];
        g->adv = FONT_DEF[i].adv;
        g->first = g_nseg;
        const char *p = FONT_DEF[i].strokes;
        float px = 0, py = 0;
        int have = 0;
        while (*p) {
            while (*p == ' ') ++p;
            if (*p == '|') { have = 0; ++p; continue; }
            if (!*p) break;
            char *e;
            float x = strtof(p, &e);
            if (*e != ',') break;
            p = e + 1;
            float y = strtof(p, &e);
            p = e;
            if (have && g_nseg < MAX_SEGS) {
                g_seg[g_nseg][0] = px; g_seg[g_nseg][1] = py;
                g_seg[g_nseg][2] = x;  g_seg[g_nseg][3] = y;
                ++g_nseg;
            }
            px = x; py = y; have = 1;
        }
        g->count = g_nseg - g->first;
    }
}

static const Glyph *glyph_for(int ch) {
    if (ch < 32 || ch > 126 || g_glyph[ch].adv == 0.0f) ch = '?';
    return &g_glyph[ch];
}

/* ========================================================================
 *  Palette: quiet, cool, restrained
 * ======================================================================== */
static const float C_TEXT[4]   = { 0.90f, 0.91f, 0.93f, 0.95f };   /* off-white         */
static const float C_LABEL[4]  = { 0.64f, 0.68f, 0.74f, 0.92f };   /* cool muted grey   */
static const float C_FAINT[4]  = { 0.62f, 0.66f, 0.72f, 0.58f };
static const float C_AMBER[4]  = { 0.93f, 0.72f, 0.44f, 1.00f };   /* soft amber        */
static const float C_CYAN[4]   = { 0.50f, 0.80f, 0.86f, 1.00f };   /* muted cyan        */
static const float C_TRACK[4]  = { 0.80f, 0.85f, 0.92f, 0.20f };
static const float C_PANEL[4]  = { 0.025f, 0.030f, 0.045f, 0.64f };
static const float C_KNOB_DARK[4] = { 0.05f, 0.05f, 0.06f, 1.0f };
static const float C_LINE[4]   = { 0.85f, 0.90f, 1.00f, 0.11f };

#define FPV 12
#define MAX_QUADS 14000
#define CAP_BODY 9.0f
#define CAP_SMALL 7.2f
#define CAP_TITLE 10.5f

/* ========================================================================
 *  Primitive emission
 * ======================================================================== */
static void put_vert(float *v, float x, float y, float lu, float lv, const float *c, float am,
                     float p0, float p1, float p2, float type) {
    v[0] = x; v[1] = y; v[2] = lu; v[3] = lv;
    v[4] = c[0]; v[5] = c[1]; v[6] = c[2]; v[7] = c[3] * am;
    v[8] = p0; v[9] = p1; v[10] = p2; v[11] = type;
}

static void write_quad(float *dst, const float px[4], const float py[4], const float lu[4], const float lv[4],
                       const float *c, float am, float p0, float p1, float p2, float type) {
    static const int idx[6] = { 0, 1, 2, 0, 2, 3 };
    for (int i = 0; i < 6; ++i) {
        int k = idx[i];
        put_vert(dst + i * FPV, px[k], py[k], lu[k], lv[k], c, am, p0, p1, p2, type);
    }
}

static void stroke(Ui *u, float x0, float y0, float x1, float y1, float hw, const float *c) {
    if (u->vertCount + 6 > u->vertCap) return;
    float dx = x1 - x0, dy = y1 - y0, L = sqrtf(dx * dx + dy * dy);
    float ux = 1.0f, uy = 0.0f;
    if (L > 1e-4f) { ux = dx / L; uy = dy / L; } else L = 0.0f;
    float nx = -uy, ny = ux;
    float m = hw + 1.5f / u->scale;
    const float cu[4] = { -m, L + m, L + m, -m };
    const float cv[4] = { -m, -m, m, m };
    float px[4], py[4];
    for (int i = 0; i < 4; ++i) {
        px[i] = x0 + ux * cu[i] + nx * cv[i];
        py[i] = y0 + uy * cu[i] + ny * cv[i];
    }
    write_quad(u->verts + (size_t)u->vertCount * FPV, px, py, cu, cv, c, u->emitAlpha, L, hw, 0.0f, 0.0f);
    u->vertCount += 6;
}

static void box_into(Ui *u, float *dst, float x, float y, float w, float h, float r, const float *c, float type) {
    float hx = w * 0.5f, hy = h * 0.5f, cx = x + hx, cy = y + hy;
    float m = 1.5f / u->scale + 1.0f;
    if (r > hx) r = hx;
    if (r > hy) r = hy;
    const float cu[4] = { -hx - m, hx + m, hx + m, -hx - m };
    const float cv[4] = { -hy - m, -hy - m, hy + m, hy + m };
    float px[4], py[4];
    for (int i = 0; i < 4; ++i) { px[i] = cx + cu[i]; py[i] = cy + cv[i]; }
    write_quad(dst, px, py, cu, cv, c, u->emitAlpha, hx, hy, r, type);
}

static void box(Ui *u, float x, float y, float w, float h, float r, const float *c, int outline) {
    if (u->vertCount + 6 > u->vertCap) return;
    box_into(u, u->verts + (size_t)u->vertCount * FPV, x, y, w, h, r, c, outline ? 2.0f : 1.0f);
    u->vertCount += 6;
}

static void dot(Ui *u, float x, float y, float radius, const float *c) {
    stroke(u, x, y, x, y, radius, c);
}

/* ---- text ---- */
static float text_width(const char *s, float cap, float track) {
    float k = cap / 7.0f, w = 0.0f;
    int n = 0;
    for (; *s; ++s, ++n) w += glyph_for((unsigned char)*s)->adv * k + track;
    return n ? w - track : 0.0f;
}

static void text(Ui *u, float x, float y, float cap, const float *c, const char *s, float track) {
    float k = cap / 7.0f;
    float base = y + cap;
    float hw = clampf(cap * 0.066f, 0.46f, 1.2f);
    for (; *s; ++s) {
        const Glyph *g = glyph_for((unsigned char)*s);
        for (int i = 0; i < g->count; ++i) {
            const float *sg = g_seg[g->first + i];
            stroke(u, x + sg[0] * k, base - sg[1] * k, x + sg[2] * k, base - sg[3] * k, hw, c);
        }
        x += g->adv * k + track;
    }
}

/* ========================================================================
 *  Setup / frame
 * ======================================================================== */
int ui_init(Ui *u) {
    memset(u, 0, sizeof *u);
    build_font();
    u->program = shader_load_program("ui.vert", "ui.frag");
    if (!u->program) return 0;

    u->vertCap = MAX_QUADS * 6;
    u->verts = (float *)malloc(sizeof(float) * FPV * (size_t)u->vertCap);
    if (!u->verts) { bhs_log("Out of memory (UI)"); return 0; }

    glGenVertexArrays(1, &u->vao);
    glGenBuffers(1, &u->vbo);
    glBindVertexArray(u->vao);
    glBindBuffer(GL_ARRAY_BUFFER, u->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * FPV * (size_t)u->vertCap), NULL, GL_DYNAMIC_DRAW);
    GLsizei stride = (GLsizei)(sizeof(float) * FPV);
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray((GLuint)i);
        glVertexAttribPointer((GLuint)i, i == 0 || i == 1 ? 2 : 4, GL_FLOAT, GL_FALSE, stride,
                              (void *)(sizeof(float) * (size_t)(i == 0 ? 0 : i == 1 ? 2 : i == 2 ? 4 : 8)));
    }
    glBindVertexArray(0);
    u->alpha = 0.0f; u->targetAlpha = 1.0f; u->emitAlpha = 1.0f;
    return 1;
}

void ui_shutdown(Ui *u) {
    free(u->verts);
    if (u->vbo) glDeleteBuffers(1, &u->vbo);
    if (u->vao) glDeleteVertexArrays(1, &u->vao);
    shader_delete(u->program);
    memset(u, 0, sizeof *u);
}

void ui_set_visible(Ui *u, int visible) { u->targetAlpha = visible ? 1.0f : 0.0f; }

void ui_begin(Ui *u, int fb_w, int fb_h, double mouse_x, double mouse_y, int mouse_down, float dt) {
    u->scale = clampf((float)fb_h / 960.0f, 0.8f, 2.5f);
    u->screenW = (float)fb_w / u->scale;
    u->screenH = (float)fb_h / u->scale;
    u->mx = (float)mouse_x / u->scale;
    u->my = (float)mouse_y / u->scale;
    u->mousePressed = mouse_down && !u->prevDown;
    u->mouseDown = mouse_down;
    u->prevDown = mouse_down;
    if (!mouse_down) u->active = 0;
    u->idCounter = 0;
    u->vertCount = 0;
    u->interactive = u->targetAlpha > 0.5f;
    u->alpha += (u->targetAlpha - u->alpha) * (1.0f - expf(-dt * 7.0f));
    u->emitAlpha = u->alpha;
    memcpy(u->prevPanels, u->panels, sizeof u->panels);
    u->prevPanelCount = u->panelCount;
    u->panelCount = 0;
}

int ui_mouse_captured(const Ui *u) {
    if (u->active) return 1;
    if (!u->interactive) return 0;
    for (int i = 0; i < u->prevPanelCount; ++i) {
        const UiRect *r = &u->prevPanels[i];
        if (u->mx >= r->x && u->mx <= r->x + r->w && u->my >= r->y && u->my <= r->y + r->h) return 1;
    }
    return 0;
}

void ui_end(Ui *u) {
    if (u->vertCount == 0) return;
    glUseProgram(u->program);
    glUniform2f(glGetUniformLocation(u->program, "uScreen"), u->screenW, u->screenH);
    glUniform1f(glGetUniformLocation(u->program, "uScale"), u->scale);
    glBindVertexArray(u->vao);
    glBindBuffer(GL_ARRAY_BUFFER, u->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(sizeof(float) * FPV * (size_t)u->vertCount), u->verts);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, u->vertCount);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void ui_hint(Ui *u, const char *s, float alpha) {
    if (alpha <= 0.01f) return;
    float saved = u->emitAlpha;
    u->emitAlpha = alpha;
    float w = text_width(s, CAP_SMALL, 0.9f);
    text(u, (u->screenW - w) * 0.5f, u->screenH - 30.0f, CAP_SMALL, C_LABEL, s, 0.9f);
    u->emitAlpha = saved;
}

/* ========================================================================
 *  Layout
 * ======================================================================== */
#define PAD 20.0f

void ui_panel_begin(Ui *u, float x, float y, float w, const char *title) {
    u->panelX = x; u->panelY = y; u->panelW = w;
    u->panelBgVert = u->vertCount;
    box(u, x, y, w, 10, 7.0f, C_PANEL, 0);               /* placeholder, resized in panel_end */
    u->panelOutlineVert = u->vertCount;
    box(u, x, y, w, 10, 7.0f, C_LINE, 1);
    u->curX = x + PAD; u->curW = w - 2 * PAD; u->curY = y + PAD;
    text(u, u->curX, u->curY, CAP_TITLE, C_TEXT, title, 0.5f);
    u->curY += CAP_TITLE + 18.0f;
}

void ui_panel_end(Ui *u) {
    float h = u->curY - u->panelY + PAD - 6.0f;
    if (u->panelBgVert + 6 <= u->vertCap) {
        box_into(u, u->verts + (size_t)u->panelBgVert * FPV, u->panelX, u->panelY, u->panelW, h, 7.0f, C_PANEL, 1.0f);
        box_into(u, u->verts + (size_t)u->panelOutlineVert * FPV, u->panelX, u->panelY, u->panelW, h, 7.0f, C_LINE, 2.0f);
    }
    if (u->panelCount < UI_MAX_PANELS) {
        UiRect r = { u->panelX, u->panelY, u->panelW, h };
        u->panels[u->panelCount++] = r;
    }
}

void ui_spacing(Ui *u, float h) { u->curY += h; }

void ui_section(Ui *u, const char *label) {
    u->curY += 10.0f;
    text(u, u->curX, u->curY, CAP_SMALL, C_FAINT, label, 1.6f);
    u->curY += CAP_SMALL + 13.0f;
}

void ui_row(Ui *u, const char *label, const char *value, int accent) {
    text(u, u->curX, u->curY, CAP_BODY, C_LABEL, label, 0.0f);
    float w = text_width(value, CAP_BODY, 0.0f);
    text(u, u->curX + u->curW - w, u->curY, CAP_BODY, accent ? C_CYAN : C_TEXT, value, 0.0f);
    u->curY += CAP_BODY + 11.0f;
}

void ui_paragraph(Ui *u, const char *s, int small) {
    float cap = small ? CAP_SMALL : CAP_BODY;
    float lead = cap + (small ? 6.0f : 7.5f);
    char line[256];
    int len = 0;
    const char *p = s;
    while (*p) {
        while (*p == ' ') ++p;
        const char *e = p;
        while (*e && *e != ' ') ++e;
        int wl = (int)(e - p);
        if (wl == 0) break;
        char cand[256];
        int cl = len;
        memcpy(cand, line, (size_t)len);
        if (cl > 0) cand[cl++] = ' ';
        for (int i = 0; i < wl && cl < 250; ++i) cand[cl++] = p[i];
        cand[cl] = '\0';
        if (len > 0 && text_width(cand, cap, 0.0f) > u->curW) {
            line[len] = '\0';
            text(u, u->curX, u->curY, cap, small ? C_FAINT : C_LABEL, line, 0.0f);
            u->curY += lead;
            len = 0;
        }
        if (len > 0) line[len++] = ' ';
        for (int i = 0; i < wl && len < 250; ++i) line[len++] = p[i];
        p = e;
    }
    if (len > 0) {
        line[len] = '\0';
        text(u, u->curX, u->curY, cap, small ? C_FAINT : C_LABEL, line, 0.0f);
        u->curY += lead;
    }
}

/* ========================================================================
 *  Widgets
 * ======================================================================== */
static int hover(const Ui *u, float x, float y, float w, float h) {
    return u->interactive && u->mx >= x && u->mx <= x + w && u->my >= y && u->my <= y + h;
}

int ui_button(Ui *u, const char *label, int icon, int accent) {
    int id = ++u->idCounter;
    float x = u->curX, y = u->curY, w = u->curW, h = 28.0f;
    int hot = hover(u, x, y, w, h), clicked = 0;
    if (hot && u->mousePressed && !u->active) { clicked = 1; u->active = id; }
    const float *tc = accent ? C_AMBER : C_TEXT;
    float fill[4] = { 1.0f, 1.0f, 1.0f, hot ? 0.07f : 0.0f };
    float edge[4] = { tc[0], tc[1], tc[2], hot ? 0.55f : 0.28f };
    if (fill[3] > 0.0f) box(u, x, y, w, h, h * 0.5f, fill, 0);
    box(u, x, y, w, h, h * 0.5f, edge, 1);

    float tw = text_width(label, CAP_BODY, 0.4f);
    float iw = icon ? 18.0f : 0.0f;
    float sx = x + (w - (tw + iw)) * 0.5f;
    float cy = y + h * 0.5f;
    if (icon == 1) {                                   /* pause */
        stroke(u, sx + 2, cy - 4, sx + 2, cy + 4, 0.9f, tc);
        stroke(u, sx + 8, cy - 4, sx + 8, cy + 4, 0.9f, tc);
    } else if (icon == 2) {                            /* play */
        stroke(u, sx + 2, cy - 4.5f, sx + 2, cy + 4.5f, 0.9f, tc);
        stroke(u, sx + 2, cy - 4.5f, sx + 9, cy, 0.9f, tc);
        stroke(u, sx + 2, cy + 4.5f, sx + 9, cy, 0.9f, tc);
    }
    text(u, sx + iw, cy - CAP_BODY * 0.5f, CAP_BODY, tc, label, 0.4f);
    u->curY += h + 10.0f;
    return clicked;
}

int ui_link(Ui *u, const char *label) {
    int id = ++u->idCounter;
    float w = text_width(label, CAP_SMALL, 1.2f);
    int hot = hover(u, u->curX - 2, u->curY - 4, w + 4, CAP_SMALL + 10), clicked = 0;
    if (hot && u->mousePressed && !u->active) { clicked = 1; u->active = id; }
    float c[4] = { C_AMBER[0], C_AMBER[1], C_AMBER[2], hot ? 1.0f : 0.72f };
    text(u, u->curX, u->curY, CAP_SMALL, c, label, 1.2f);
    u->curY += CAP_SMALL + 12.0f;
    return clicked;
}

int ui_toggle(Ui *u, const char *label, int *value) {
    int id = ++u->idCounter;
    float x = u->curX, y = u->curY, w = u->curW;
    int hot = hover(u, x, y - 4, w, 22), changed = 0;
    if (hot && u->mousePressed && !u->active) { *value = !*value; changed = 1; u->active = id; }
    text(u, x, y + 1, CAP_BODY, *value ? C_TEXT : C_LABEL, label, 0.0f);
    float sw = 26.0f, sh = 13.0f, sx = x + w - sw, sy = y - 1.0f;
    float edge[4] = { 0.85f, 0.90f, 1.0f, hot ? 0.45f : 0.25f };
    if (*value) {
        float f[4] = { C_AMBER[0], C_AMBER[1], C_AMBER[2], 0.85f };
        box(u, sx, sy, sw, sh, sh * 0.5f, f, 0);
        dot(u, sx + sw - sh * 0.5f, sy + sh * 0.5f, 4.3f, C_KNOB_DARK);
    } else {
        box(u, sx, sy, sw, sh, sh * 0.5f, edge, 1);
        dot(u, sx + sh * 0.5f, sy + sh * 0.5f, 3.6f, C_LABEL);
    }
    u->curY += 25.0f;
    return changed;
}

int ui_slider(Ui *u, const char *label, float *value, float lo, float hi, const char *fmt) {
    int id = ++u->idCounter;
    float x = u->curX, y = u->curY, w = u->curW;
    float ty = y + CAP_BODY + 11.0f;                    /* track centre line */
    int hot = hover(u, x - 6, ty - 10, w + 12, 20);
    int changed = 0;
    if (hot && u->mousePressed && !u->active) u->active = id;
    if (u->active == id && u->mouseDown) {
        *value = lo + clampf((u->mx - x) / w, 0.0f, 1.0f) * (hi - lo);
        changed = 1;
    }
    float t = clampf((*value - lo) / (hi - lo), 0.0f, 1.0f);
    int on = hot || u->active == id;

    char buf[48];
    snprintf(buf, sizeof buf, fmt, *value);
    text(u, x, y, CAP_BODY, on ? C_TEXT : C_LABEL, label, 0.0f);
    text(u, x + w - text_width(buf, CAP_BODY, 0.0f), y, CAP_BODY, C_TEXT, buf, 0.0f);

    stroke(u, x, ty, x + w, ty, 0.55f, C_TRACK);
    if (t > 0.002f) stroke(u, x, ty, x + w * t, ty, 0.75f, C_AMBER);
    dot(u, x + w * t, ty, on ? 4.6f : 3.6f, on ? C_AMBER : C_TEXT);
    u->curY += CAP_BODY + 11.0f + 17.0f;
    return changed;
}
