#include "errorlog.h"
#include "particles.h"
#include "shader.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FLOATS_PER_VERT 8   /* x y z | r g b a | size */
#define TWO_PI 6.28318530718f

static float frand(ParticleSystem *ps) {            /* xorshift32 -> [0,1) */
    unsigned int x = ps->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    ps->rng = x;
    return (float)(x & 0xFFFFFFu) / 16777216.0f;
}

/* CPU copy of the shader's artistic blackbody palette (t: 0 red .. 1.5 blue-white). */
static void heat_color(float t, float *rgb) {
    float c[3] = { 0.20f, 0.015f, 0.0f };
    static const float k1[3] = { 1.00f, 0.27f, 0.02f };
    static const float k2[3] = { 1.00f, 0.72f, 0.22f };
    static const float k3[3] = { 1.00f, 0.93f, 0.80f };
    static const float k4[3] = { 0.62f, 0.76f, 1.00f };
    const float *ks[4] = { k1, k2, k3, k4 };
    const float lo[4] = { 0.0f, 0.30f, 0.60f, 0.95f };
    const float hi[4] = { 0.35f, 0.65f, 0.95f, 1.50f };
    for (int s = 0; s < 4; ++s) {
        float x = clampf((t - lo[s]) / (hi[s] - lo[s]), 0.0f, 1.0f);
        x = x * x * (3.0f - 2.0f * x);
        for (int i = 0; i < 3; ++i) c[i] += (ks[s][i] - c[i]) * x;
    }
    rgb[0] = c[0]; rgb[1] = c[1]; rgb[2] = c[2];
}

static void spawn(ParticleSystem *ps, Particle *q, int anywhere) {
    float u = frand(ps);
    q->inward = frand(ps) < 0.25f ? 0.02f + 0.08f * frand(ps) : 0.0f; /* 25% spiral in */
    float rmin = DISK_INNER_RS * 0.9f, rmax = DISK_OUTER_RS * 1.15f;
    q->r = anywhere ? rmin + (rmax - rmin) * powf(u, 1.5f)
                    : rmax * (0.80f + 0.20f * frand(ps));
    q->phi = frand(ps) * TWO_PI;
    q->yrel = (frand(ps) + frand(ps) + frand(ps) - 1.5f) * 0.07f;
    q->speed = 0.85f + 0.30f * frand(ps);
    q->temp = frand(ps);
    q->size = 0.035f + 0.085f * powf(frand(ps), 3.0f);
    q->phase = frand(ps) * TWO_PI;
}

int particles_active_count(const SimParams *sim) {
    return (int)(clampf(sim->particleDensity, 0.0f, 1.0f) * PARTICLE_CAPACITY + 0.5f);
}

void particles_reset(ParticleSystem *ps, const SimParams *sim) {
    (void)sim;
    ps->rng = 0x9E3779B9u;
    ps->clock = 0.0f;
    for (int i = 0; i < PARTICLE_CAPACITY; ++i) spawn(ps, &ps->p[i], 1);
}

int particles_init(ParticleSystem *ps, const SimParams *sim) {
    memset(ps, 0, sizeof *ps);
    ps->p = (Particle *)malloc(sizeof(Particle) * PARTICLE_CAPACITY);
    ps->verts = (float *)malloc(sizeof(float) * FLOATS_PER_VERT * 3 * PARTICLE_CAPACITY);
    if (!ps->p || !ps->verts) { bhs_log("Out of memory (particles)"); return 0; }
    particles_reset(ps, sim);

    ps->program = shader_load_program("particle.vert", "particle.frag");
    if (!ps->program) return 0;

    glGenVertexArrays(1, &ps->vao);
    glGenBuffers(1, &ps->vbo);
    glBindVertexArray(ps->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ps->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(sizeof(float) * FLOATS_PER_VERT * 3 * PARTICLE_CAPACITY), NULL, GL_DYNAMIC_DRAW);
    GLsizei stride = (GLsizei)(sizeof(float) * FLOATS_PER_VERT);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void *)(sizeof(float) * 3));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void *)(sizeof(float) * 7));
    glBindVertexArray(0);
    return 1;
}

/* Advance the orbits: Keplerian rotation, plus inward spiral for some particles. */
void particles_update(ParticleSystem *ps, float sim_dt, const SimParams *sim) {
    int n = particles_active_count(sim);
    ps->active = n;
    ps->clock += sim_dt;
    float dtn = sim_dt / sim->bhSize;              /* time in units of rs/c */
    for (int i = 0; i < n; ++i) {
        Particle *q = &ps->p[i];
        float omega = sqrtf(0.5f / (q->r * q->r * q->r));       /* normalised Kepler rate */
        q->phi -= omega * q->speed * sim->diskSpeed * dtn;       /* same spin sense as shader */
        if (q->inward > 0.0f) {
            float k = DISK_INNER_RS / q->r;
            q->r -= q->inward * (1.0f + k * k) * dtn;            /* faster plunge when close */
            if (q->r < 1.1f) spawn(ps, q, 0);                    /* swallowed -> respawn outside */
        }
        if (q->phi < -TWO_PI) q->phi += TWO_PI;
    }
}

void particles_render(ParticleSystem *ps, const Camera *cam, const SimParams *sim, int width, int height) {
    int n = ps->active;
    if (n <= 0) return;
    float rs = sim->bhSize;
    float *heads = ps->verts;
    float *lines = ps->verts + (size_t)n * FLOATS_PER_VERT;

    for (int i = 0; i < n; ++i) {
        const Particle *q = &ps->p[i];
        float omega = sqrtf(0.5f / (q->r * q->r * q->r));
        float trail = omega * q->speed * sim->diskSpeed * 0.45f;  /* trail length (rad) */
        float cp = cosf(q->phi), sp = sinf(q->phi);
        float ct = cosf(q->phi + trail), st = sinf(q->phi + trail);
        float R = q->r * rs, Y = q->yrel * q->r * rs;

        float heat = clampf(0.25f + 0.55f * sqrtf(DISK_INNER_RS / q->r) + 0.40f * (q->temp - 0.5f), 0.0f, 1.4f);
        float rgb[3];
        heat_color(heat, rgb);
        float pulse = 0.35f + 0.65f * (0.5f + 0.5f * sinf(q->phase + ps->clock * (0.5f + q->temp)));
        float fade = clampf((q->r - 1.1f) / 0.9f, 0.0f, 1.0f);        /* fade near the horizon */
        float a = pulse * fade * 0.7f;

        float *h = heads + (size_t)i * FLOATS_PER_VERT;
        h[0] = R * cp; h[1] = Y; h[2] = R * sp;
        h[3] = rgb[0]; h[4] = rgb[1]; h[5] = rgb[2]; h[6] = a; h[7] = q->size * rs;

        float *l = lines + (size_t)i * 2 * FLOATS_PER_VERT;
        memcpy(l, h, sizeof(float) * FLOATS_PER_VERT);
        l[6] = a * 0.55f;
        l[7] = 1.0f;
        float *t = l + FLOATS_PER_VERT;
        t[0] = R * ct; t[1] = Y; t[2] = R * st;
        t[3] = rgb[0]; t[4] = rgb[1]; t[5] = rgb[2]; t[6] = 0.0f; t[7] = 1.0f;
    }

    GLuint p = ps->program;
    glUseProgram(p);
    float aspect = (float)width / (float)(height > 0 ? height : 1);
    mat4 vp = camera_view_proj(cam, aspect);
    glUniformMatrix4fv(glGetUniformLocation(p, "uViewProj"), 1, GL_FALSE, vp.m);
    glUniform3f(glGetUniformLocation(p, "uCamPos"), cam->pos.x, cam->pos.y, cam->pos.z);
    glUniform1f(glGetUniformLocation(p, "uRs"), rs);
    glUniform1f(glGetUniformLocation(p, "uPixelScale"), (float)height / (2.0f * tanf(cam->fovY * 0.5f)));
    glUniform1f(glGetUniformLocation(p, "uBright"), 1.4f);

    glBindVertexArray(ps->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ps->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(sizeof(float) * FLOATS_PER_VERT * 3 * (size_t)n), ps->verts);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);                   /* additive glow */
    glEnable(GL_PROGRAM_POINT_SIZE);

    glUniform1f(glGetUniformLocation(p, "uIsPoint"), 0.0f);
    glDrawArrays(GL_LINES, n, 2 * n);              /* trails  */
    glUniform1f(glGetUniformLocation(p, "uIsPoint"), 1.0f);
    glDrawArrays(GL_POINTS, 0, n);                 /* heads   */

    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void particles_shutdown(ParticleSystem *ps) {
    free(ps->p); free(ps->verts);
    ps->p = NULL; ps->verts = NULL;
    if (ps->vbo) glDeleteBuffers(1, &ps->vbo);
    if (ps->vao) glDeleteVertexArrays(1, &ps->vao);
    shader_delete(ps->program);
    ps->vbo = ps->vao = ps->program = 0;
}
