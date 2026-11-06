#ifndef CAMERA_H
#define CAMERA_H
#include "vecmath.h"

/* Orbit camera with smooth interpolation. "t*" fields are targets set by the
 * input code; the plain fields are what is rendered (they chase the targets). */
typedef struct Camera {
    float yaw, pitch, dist;  vec3 center;      /* current (smoothed)   */
    float tYaw, tPitch, tDist; vec3 tCenter;   /* targets              */
    vec3 pos, fwd, right, up;                  /* derived each update  */
    float fovY;                                /* radians              */
} Camera;

void camera_reset(Camera *c, float rs, int snap);
void camera_rotate(Camera *c, float dx_pixels, float dy_pixels);
void camera_zoom(Camera *c, float scroll_steps);
void camera_pan(Camera *c, float dx_pixels, float dy_pixels);
void camera_move(Camera *c, float forward, float right, float up); /* world units */
void camera_update(Camera *c, float dt, float min_distance);
mat4 camera_view_proj(const Camera *c, float aspect);

#endif
