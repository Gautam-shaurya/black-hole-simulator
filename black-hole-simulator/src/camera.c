#include "camera.h"
#include <math.h>

#define DEFAULT_DIST_RS 24.0f
#define DEFAULT_PITCH 0.20f
#define MAX_DIST_RS 120.0f

static vec3 dir_from_angles(float yaw, float pitch) {
    float cp = cosf(pitch);
    return v3(cp * sinf(yaw), sinf(pitch), cp * cosf(yaw));
}

void camera_reset(Camera *c, float rs, int snap) {
    c->tYaw = 0.0f;
    c->tPitch = DEFAULT_PITCH;
    c->tDist = DEFAULT_DIST_RS * rs;
    c->tCenter = v3(0.0f, 0.0f, 0.0f);
    c->fovY = 0.785f; /* 45 degrees */
    if (snap) {
        c->yaw = c->tYaw; c->pitch = c->tPitch;
        c->dist = c->tDist; c->center = c->tCenter;
    }
}

void camera_rotate(Camera *c, float dx, float dy) {
    c->tYaw -= dx * 0.005f;                    /* drag right -> world turns right */
    c->tPitch = clampf(c->tPitch + dy * 0.005f, -1.5f, 1.5f);
}

void camera_zoom(Camera *c, float steps) {
    c->tDist *= expf(-steps * 0.10f);
    c->tDist = clampf(c->tDist, 0.5f, 400.0f);
}

void camera_pan(Camera *c, float dx, float dy) {
    float k = c->tDist * 0.0012f;
    c->tCenter = v3_add(c->tCenter, v3_add(v3_scale(c->right, -dx * k), v3_scale(c->up, dy * k)));
}

void camera_move(Camera *c, float forward, float right, float up) {
    vec3 d = v3_add(v3_scale(c->fwd, forward), v3_add(v3_scale(c->right, right), v3_scale(c->up, up)));
    c->tCenter = v3_add(c->tCenter, d);
}

void camera_update(Camera *c, float dt, float min_distance) {
    /* 1. Keep the TARGET outside the exclusion sphere around the hole, so the
     *    camera never tries to fly into the event horizon. */
    vec3 tdir = dir_from_angles(c->tYaw, c->tPitch);
    if (v3_length(c->tCenter) < 1e-3f && c->tDist < min_distance) c->tDist = min_distance;
    vec3 tpos = v3_add(c->tCenter, v3_scale(tdir, c->tDist));
    float l = v3_length(tpos);
    if (l < min_distance) {
        vec3 n = l > 1e-4f ? v3_scale(tpos, 1.0f / l) : tdir;
        tpos = v3_scale(n, min_distance);
        c->tCenter = v3_sub(tpos, v3_scale(tdir, c->tDist));
    }

    /* 2. Exponential smoothing toward the targets (frame-rate independent). */
    float k = 1.0f - expf(-dt * 12.0f);
    c->yaw    += (c->tYaw - c->yaw) * k;
    c->pitch  += (c->tPitch - c->pitch) * k;
    c->dist   += (c->tDist - c->dist) * k;
    c->center = v3_add(c->center, v3_scale(v3_sub(c->tCenter, c->center), k));

    /* 3. Derived vectors, with a last-resort safety push-out. */
    vec3 dir = dir_from_angles(c->yaw, c->pitch);
    c->pos = v3_add(c->center, v3_scale(dir, c->dist));
    l = v3_length(c->pos);
    if (l < min_distance) c->pos = v3_scale(l > 1e-4f ? v3_scale(c->pos, 1.0f / l) : dir, min_distance);

    vec3 to_center = v3_sub(c->center, c->pos);
    c->fwd = v3_length(to_center) > 1e-4f ? v3_normalize(to_center) : v3_scale(dir, -1.0f);
    c->right = v3_normalize(v3_cross(c->fwd, v3(0.0f, 1.0f, 0.0f)));
    c->up = v3_cross(c->right, c->fwd);
}

mat4 camera_view_proj(const Camera *c, float aspect) {
    mat4 view = mat4_look_at(c->pos, v3_add(c->pos, c->fwd), v3(0.0f, 1.0f, 0.0f));
    mat4 proj = mat4_perspective(c->fovY, aspect, 0.05f, 2000.0f);
    return mat4_mul(proj, view);
}
