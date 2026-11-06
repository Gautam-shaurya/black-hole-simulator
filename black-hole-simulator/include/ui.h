/* ui.h - minimal, light-weight immediate-mode UI.
 * Text is drawn with a thin vector stroke font, shapes with signed-distance
 * shaders, so everything is anti-aliased and crisp at any resolution.
 * Coordinates are "logical units"; the UI scales smoothly with window height. */
#ifndef UI_H
#define UI_H
#include "gl_loader.h"

#define UI_MAX_PANELS 4

typedef struct UiRect { float x, y, w, h; } UiRect;

typedef struct Ui {
    GLuint program, vao, vbo;
    float *verts;
    int vertCount, vertCap;

    float scale;                 /* framebuffer pixels per logical unit */
    float screenW, screenH;      /* logical size                        */
    float mx, my;                /* mouse, logical                      */
    int mouseDown, mousePressed, prevDown;
    int active;                  /* id of the widget being dragged      */
    int idCounter;
    int interactive;             /* widgets respond only when UI is shown */
    float alpha, targetAlpha;    /* fade in / out                       */
    float emitAlpha;             /* multiplier applied to emitted shapes */

    UiRect panels[UI_MAX_PANELS], prevPanels[UI_MAX_PANELS];
    int panelCount, prevPanelCount, panelBgVert, panelOutlineVert;
    float panelX, panelY, panelW;
    float curX, curY, curW;      /* layout cursor */
} Ui;

int  ui_init(Ui *u);
void ui_shutdown(Ui *u);

/* mouse position in framebuffer pixels */
void ui_begin(Ui *u, int fb_w, int fb_h, double mouse_x, double mouse_y, int mouse_down, float dt);
void ui_end(Ui *u);

int  ui_mouse_captured(const Ui *u);
void ui_set_visible(Ui *u, int visible);

void ui_panel_begin(Ui *u, float x, float y, float w, const char *title);
void ui_panel_end(Ui *u);

void ui_spacing(Ui *u, float h);
void ui_section(Ui *u, const char *label);                       /* small tracked caption */
void ui_row(Ui *u, const char *label, const char *value, int accent);
void ui_paragraph(Ui *u, const char *text, int small);
int  ui_button(Ui *u, const char *label, int icon /*0 none,1 pause,2 play*/, int accent);
int  ui_link(Ui *u, const char *label);
int  ui_toggle(Ui *u, const char *label, int *value);
int  ui_slider(Ui *u, const char *label, float *value, float lo, float hi, const char *fmt);

/* faint centred hint at the bottom of the screen; alpha 0..1, drawn even when panels are hidden */
void ui_hint(Ui *u, const char *text, float alpha);
#endif
