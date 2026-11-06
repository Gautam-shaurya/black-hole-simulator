/* main.c - window, input, UI layout and the render loop.
 *
 * Frame pipeline:
 *   1. black hole ray-march (stars, lensing, disk, photon ring) -> HDR texture
 *   2. particles (additive points + trails)                      -> HDR texture
 *   3. bloom chain + tone mapping + grading + vignette           -> window
 *   4. user interface overlay                                    -> window
 */
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "accretion_disk.h"
#include "blackhole.h"
#include "errorlog.h"
#include "shader.h"
#include "camera.h"
#include "gl_loader.h"
#include "particles.h"
#include "physics.h"
#include "postfx.h"
#include "ui.h"

typedef struct App {
    GLFWwindow *window;
    int fbw, fbh;
    Camera cam;
    SimParams sim;
    BlackHole bh;
    ParticleSystem ps;
    PostFX fx;
    Ui ui;

    double scrollAccum;
    double lastMx, lastMy;
    int dragLeft, dragMid, prevLeft, prevMid;
    float cinBlend;       /* smooth 0..1 follower of sim.cinematic */
    int topic;            /* educational topic currently shown     */
    int infoOpen;         /* "About" section expanded?             */
    int uiShown;          /* H key: panels on/off                  */
    double lastMove, pmx, pmy, startTime;
    float fps;
    int running;
} App;

static const char *k_topic_names[4] = {
    "Event horizon", "Accretion disk", "Gravitational lensing", "Why light cannot escape"
};
static const char *k_topic_text[4] = {
    "The event horizon is the boundary around a black hole beyond which nothing can return. "
    "Its size is set by the mass of the hole. Inside it, every future path leads inward.",
    "Gas and dust spiral around a black hole in a flat, rotating disk. Friction heats it until it glows. "
    "The inner gas orbits fastest, so it is hotter and brighter.",
    "Mass bends spacetime, so light follows curved paths near a black hole. Background stars appear "
    "stretched or duplicated, and the disk behind the hole seems to wrap over and under it.",
    "Inside the horizon, spacetime curves so strongly that every possible path, even a beam of light, "
    "points toward the center. Escaping would need a speed above that of light."
};

/* ------------------------------------------------------------ helpers --- */
static void reset_simulation(App *a) {
    sim_defaults(&a->sim);
    particles_reset(&a->ps, &a->sim);
    camera_reset(&a->cam, a->sim.bhSize, 0);
    a->cinBlend = 0.0f;
}

static void error_callback(int code, const char *msg) {
    bhs_log("GLFW error %d: %s", code, msg);
}

static void framebuffer_cb(GLFWwindow *w, int width, int height) {
    (void)w; (void)width; (void)height; /* handled in the main loop */
}

static void scroll_cb(GLFWwindow *w, double dx, double dy) {
    (void)dx;
    App *a = (App *)glfwGetWindowUserPointer(w);
    a->scrollAccum += dy;
}

static void key_cb(GLFWwindow *w, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods;
    App *a = (App *)glfwGetWindowUserPointer(w);
    if (action != GLFW_PRESS) return;
    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(w, GLFW_TRUE); break;
        case GLFW_KEY_R:      camera_reset(&a->cam, a->sim.bhSize, 0); break;
        case GLFW_KEY_SPACE:  a->sim.paused = !a->sim.paused; break;
        case GLFW_KEY_C:      a->sim.cinematic = !a->sim.cinematic; break;
        case GLFW_KEY_H:      a->uiShown = !a->uiShown; break;
        default: break;
    }
}

static void *get_proc(const char *name) { return (void *)glfwGetProcAddress(name); }

/* ------------------------------------------------------------- UI ------- */
static void build_ui(App *a) {
    Ui *u = &a->ui;
    SimParams *s = &a->sim;
    char b[64];
    const float W = 244.0f, M = 24.0f;

    /* ---------- left: controls ---------- */
    ui_panel_begin(u, M, M, W, "Simulation");
    if (ui_button(u, s->paused ? "Resume" : "Pause", s->paused ? 2 : 1, 0)) s->paused = !s->paused;
    ui_slider(u, "Speed", &s->timeScale, 0.0f, 4.0f, "%.2fx");
    ui_slider(u, "Brightness", &s->diskBrightness, 0.0f, 3.0f, "%.2f");
    ui_slider(u, "Particles", &s->particleDensity, 0.0f, 1.0f, "%.2f");
    ui_slider(u, "Lensing", &s->lensing, 0.0f, 2.0f, "%.2f");
    ui_slider(u, "Size", &s->bhSize, 0.5f, 2.0f, "%.2f SU");
    ui_slider(u, "Disk rotation", &s->diskSpeed, 0.0f, 4.0f, "%.2fx");
    ui_section(u, "LAYERS");
    ui_toggle(u, "Starfield", &s->starfield);
    ui_toggle(u, "Particles", &s->particles);
    ui_toggle(u, "Gravitational lensing", &s->lensingOn);
    ui_toggle(u, "Cinematic view", &s->cinematic);
    ui_spacing(u, 6);
    if (ui_button(u, "Reset simulation", 0, 1)) reset_simulation(a);
    ui_panel_end(u);

    /* ---------- right: live data ---------- */
    float rs = s->bhSize;
    ui_panel_begin(u, u->screenW - W - M, M, W, "System data");
    snprintf(b, sizeof b, "%.2f SU", rs);                                ui_row(u, "Radius", b, 0);
    snprintf(b, sizeof b, "%.2f SU", 2.0f * rs);                         ui_row(u, "Horizon width", b, 0);
    snprintf(b, sizeof b, "%.2f SU", physics_photon_sphere(rs));         ui_row(u, "Photon sphere", b, 0);
    snprintf(b, sizeof b, "%.3f rad/SU", accretion_disk_inner_omega(s)); ui_row(u, "Disk spin", b, 0);
    snprintf(b, sizeof b, "%d", s->particles ? particles_active_count(s) : 0); ui_row(u, "Particles", b, 0);
    snprintf(b, sizeof b, "%.1f SU", s->simTime);                        ui_row(u, "Time", b, 0);
    snprintf(b, sizeof b, "%.1f SU", v3_length(a->cam.pos));             ui_row(u, "Distance", b, 0);
    ui_row(u, "Lensing", s->lensingOn ? "Active" : "Off", s->lensingOn);
    snprintf(b, sizeof b, "%.0f", a->fps);                               ui_row(u, "FPS", b, 0);

    ui_section(u, "ABOUT");
    if (!a->infoOpen) {
        if (ui_link(u, "LEARN MORE  +")) a->infoOpen = 1;
    } else {
        ui_paragraph(u, k_topic_names[a->topic], 0);
        ui_spacing(u, 2);
        ui_paragraph(u, k_topic_text[a->topic], 1);
        ui_spacing(u, 4);
        if (ui_link(u, "NEXT TOPIC  >")) a->topic = (a->topic + 1) % 4;
        if (ui_link(u, "HIDE  -")) a->infoOpen = 0;
    }
    ui_spacing(u, 2);
    ui_paragraph(u, "Values are in simulation units (SU), not real measurements. Lensing is an approximation.", 1);
    ui_panel_end(u);
}

/* ---------------------------------------------------------- input ------- */
static void handle_input(App *a, float dt) {
    GLFWwindow *w = a->window;
    double mx, my;
    int winw, winh;
    glfwGetCursorPos(w, &mx, &my);
    glfwGetWindowSize(w, &winw, &winh);
    double sx = winw > 0 ? (double)a->fbw / winw : 1.0, sy = winh > 0 ? (double)a->fbh / winh : 1.0;
    mx *= sx; my *= sy;                                  /* window coords -> framebuffer pixels */

    int left = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    int mid  = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    int over_ui = ui_mouse_captured(&a->ui);

    if (left && !a->prevLeft) a->dragLeft = !over_ui;    /* a drag that starts on the UI never rotates */
    if (!left) a->dragLeft = 0;
    if (mid && !a->prevMid) a->dragMid = !over_ui;
    if (!mid) a->dragMid = 0;
    a->prevLeft = left; a->prevMid = mid;

    float dx = (float)(mx - a->lastMx), dy = (float)(my - a->lastMy);
    a->lastMx = mx; a->lastMy = my;
    if (a->dragLeft) camera_rotate(&a->cam, dx, dy);
    if (a->dragMid)  camera_pan(&a->cam, dx, dy);

    if (a->scrollAccum != 0.0) {
        if (!over_ui) camera_zoom(&a->cam, (float)a->scrollAccum);
        a->scrollAccum = 0.0;
    }

    /* WASDQE fly: speed grows with distance so it feels the same at any zoom */
    float speed = (0.35f * a->cam.tDist + 1.0f) * dt;
    float f = 0, r = 0, up = 0;
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) f += speed;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) f -= speed;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) r += speed;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) r -= speed;
    if (glfwGetKey(w, GLFW_KEY_E) == GLFW_PRESS) up += speed;
    if (glfwGetKey(w, GLFW_KEY_Q) == GLFW_PRESS) up -= speed;
    if (f != 0 || r != 0 || up != 0) camera_move(&a->cam, f, r, up);

    /* cinematic mode: slow automatic orbit with a gentle breathing sway */
    if (a->sim.cinematic && !a->dragLeft) {
        a->cam.tYaw += dt * 0.06f;
        a->cam.tPitch = 0.20f + 0.08f * sinf((float)glfwGetTime() * 0.15f);
    }
}

/* ------------------------------------------------------------- main ----- */
int main(void) {
    bhs_platform_init();
    App *a = (App *)calloc(1, sizeof *a);
    if (!a) { bhs_fatal("Black Hole Simulator", "Not enough memory to start."); return 1; }

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        bhs_fatal("Black Hole Simulator",
                  "The graphics system could not be started.\n\nPlease update your graphics driver and try again.");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);   /* required on macOS, harmless elsewhere */

    /* start with a window that fits the screen */
    int ww = 1600, wh = 900;
    GLFWmonitor *mon = glfwGetPrimaryMonitor();
    const GLFWvidmode *vm = mon ? glfwGetVideoMode(mon) : NULL;
    if (vm) {
        if (ww > (int)(vm->width * 0.92)) ww = (int)(vm->width * 0.92);
        if (wh > (int)(vm->height * 0.85)) wh = (int)(vm->height * 0.85);
    }
    if (ww < 800) ww = 800;
    if (wh < 500) wh = 500;

    a->window = glfwCreateWindow(ww, wh, "Black Hole Simulator", NULL, NULL);
    if (!a->window) {
        bhs_fatal("Black Hole Simulator - graphics problem",
                  "Your computer could not open an OpenGL 3.3 window.\n\n"
                  "This almost always means the graphics driver is missing or too old.\n\n"
                  "What to do:\n"
                  "1. Update your graphics driver (NVIDIA, AMD or Intel website, or Windows Update).\n"
                  "2. On a laptop with two graphics chips, set this program to use the high-performance one.\n"
                  "3. This program does not work inside Remote Desktop or most virtual machines.");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(a->window);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(a->window, a);
    glfwSetFramebufferSizeCallback(a->window, framebuffer_cb);
    glfwSetScrollCallback(a->window, scroll_cb);
    glfwSetKeyCallback(a->window, key_cb);

    /* Load the GL functions first (glGetString itself must be loaded). */
    int missing = gl_load(get_proc);
    const char *ver = glGetString ? (const char *)glGetString(GL_VERSION) : NULL;
    const char *ren = glGetString ? (const char *)glGetString(GL_RENDERER) : NULL;
    int vmaj = 0, vmin = 0;
    if (ver && sscanf(ver, "%d.%d", &vmaj, &vmin) == 2 && (vmaj < 3 || (vmaj == 3 && vmin < 3))) {
        bhs_log("OpenGL version %s on %s", ver, ren ? ren : "?");
        bhs_fatal("Black Hole Simulator - graphics problem",
                  "Your graphics card reports OpenGL %s, but version 3.3 or newer is needed.\n\n"
                  "Please update your graphics driver.", ver);
        glfwDestroyWindow(a->window); glfwTerminate();
        return 1;
    }
    if (missing != 0) {
        bhs_log("OpenGL version %s on %s", ver ? ver : "?", ren ? ren : "?");
        bhs_fatal("Black Hole Simulator - graphics problem",
                  "Some required graphics functions are missing.\n\nPlease update your graphics driver.");
        glfwDestroyWindow(a->window); glfwTerminate();
        return 1;
    }
    printf("OpenGL %s | %s\n", ver ? ver : "?", ren ? ren : "?");

    glfwGetFramebufferSize(a->window, &a->fbw, &a->fbh);
    sim_defaults(&a->sim);
    camera_reset(&a->cam, a->sim.bhSize, 1);

    int ok = blackhole_init(&a->bh) && particles_init(&a->ps, &a->sim) &&
             postfx_init(&a->fx, a->fbw > 0 ? a->fbw : 1, a->fbh > 0 ? a->fbh : 1) && ui_init(&a->ui);
    if (!ok) {
        bhs_log("OpenGL version %s on %s", ver ? ver : "?", ren ? ren : "?");
        if (shader_last_failure_was_missing_file())
            bhs_fatal("Black Hole Simulator - files missing",
                      "A program file could not be found.\n\n"
                      "The ZIP was probably not fully extracted. The folder named \"shaders\" must sit "
                      "next to the program. (Right-click the ZIP, choose Extract All, then open the new folder.)");
        else
            bhs_fatal("Black Hole Simulator - could not start",
                      "A graphics program (shader) failed to compile on your graphics card.\n\n"
                      "This is a problem in the simulator, not something you did wrong. "
                      "Please send the file error.log to the person who gave you the simulator. "
                      "Updating your graphics driver may also help.");
        goto cleanup;
    }

    double last = glfwGetTime(), fps_t = last, title_t = last;
    int frames = 0;
    a->running = 1;
    a->uiShown = 1;
    a->startTime = last;
    a->lastMove = -100.0;

    while (!glfwWindowShouldClose(a->window)) {
        glfwPollEvents();
        double now = glfwGetTime();
        float dt = (float)(now - last);
        last = now;
        if (dt > 0.1f) dt = 0.1f;

        /* FPS counter (smoothed over 0.4 s) */
        ++frames;
        if (now - fps_t >= 0.4) { a->fps = (float)(frames / (now - fps_t)); frames = 0; fps_t = now; }
        if (now - title_t >= 0.5) {
            char title[96];
            snprintf(title, sizeof title, "Black Hole Simulator - %.0f FPS", a->fps);
            glfwSetWindowTitle(a->window, title);
            title_t = now;
        }

        /* window size / minimise handling */
        glfwGetFramebufferSize(a->window, &a->fbw, &a->fbh);
        if (a->fbw <= 0 || a->fbh <= 0) { glfwSwapBuffers(a->window); continue; }
        if (!postfx_resize(&a->fx, a->fbw, a->fbh)) { bhs_log("Resize failed"); break; }

        /* ---- simulation ---- */
        handle_input(a, dt);
        float sdt = sim_step(&a->sim, dt);
        a->sim.simTime += sdt;
        particles_update(&a->ps, sdt, &a->sim);
        camera_update(&a->cam, dt, physics_min_camera_distance(a->sim.bhSize));
        a->cinBlend += ((a->sim.cinematic ? 1.0f : 0.0f) - a->cinBlend) * (1.0f - expf(-dt * 2.5f));

        /* ---- UI (built before drawing so input of this frame is consumed) ---- */
        double mx, my;
        int ww, wh;
        glfwGetCursorPos(a->window, &mx, &my);
        glfwGetWindowSize(a->window, &ww, &wh);
        if (ww > 0 && wh > 0) { mx *= (double)a->fbw / ww; my *= (double)a->fbh / wh; }
        ui_set_visible(&a->ui, a->uiShown && !a->sim.cinematic);
        ui_begin(&a->ui, a->fbw, a->fbh, mx, my,
                 glfwGetMouseButton(a->window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS, dt);
        if (fabs(mx - a->pmx) + fabs(my - a->pmy) > 1.0) a->lastMove = now;
        a->pmx = mx; a->pmy = my;
        if (a->ui.alpha > 0.01f) build_ui(a);
        if (a->sim.cinematic) {                      /* pristine view: only a brief hint on mouse movement */
            double since = now - a->lastMove;
            ui_hint(&a->ui, "Press C to show controls", clampf((float)(2.5 - since), 0.0f, 1.0f) * 0.9f);
        } else if (a->uiShown && now - a->startTime < 11.0) {
            double t = now - a->startTime;
            ui_hint(&a->ui, "Drag to orbit     Scroll to zoom     C cinematic view     H hide panels",
                    clampf((float)(10.5 - t), 0.0f, 1.0f) * clampf((float)(t - 0.5), 0.0f, 1.0f));
        }

        /* ---- render ---- */
        postfx_begin_scene(&a->fx);
        blackhole_render(&a->bh, &a->cam, &a->sim, a->fbw, a->fbh);
        if (a->sim.particles) particles_render(&a->ps, &a->cam, &a->sim, a->fbw, a->fbh);

        PostParams pp;
        pp.bloomStrength = 0.55f + 0.25f * a->cinBlend;
        pp.exposure = 1.0f;
        pp.vignette = 0.35f + 0.30f * a->cinBlend;
        pp.cinematic = a->cinBlend;
        pp.letterbox = 0.085f * a->cinBlend;
        pp.time = (float)now;
        postfx_present(&a->fx, &pp);

        ui_end(&a->ui);
        glfwSwapBuffers(a->window);
    }

cleanup:
    /* release every GPU resource before the context disappears */
    ui_shutdown(&a->ui);
    postfx_shutdown(&a->fx);
    particles_shutdown(&a->ps);
    blackhole_shutdown(&a->bh);
    glfwDestroyWindow(a->window);
    glfwTerminate();
    free(a);
    return ok ? 0 : 1;
}
