#include "physics.h"
#include <math.h>

void sim_defaults(SimParams *s) {
    s->paused = 0;
    s->timeScale = 1.0f;
    s->diskBrightness = 1.0f;
    s->particleDensity = 0.35f;
    s->lensing = 1.0f;
    s->bhSize = 1.0f;
    s->diskSpeed = 1.0f;
    s->starfield = 1;
    s->particles = 1;
    s->lensingOn = 1;
    s->cinematic = 0;
    s->simTime = 0.0;
}

float sim_step(const SimParams *s, float real_dt) {
    return s->paused ? 0.0f : real_dt * s->timeScale * SIM_RATE;
}

float physics_photon_sphere(float rs) { return 1.5f * rs; }
float physics_shadow_radius(float rs) { return 1.5f * sqrtf(3.0f) * rs; }
float physics_isco(float rs) { return DISK_INNER_RS * rs; }
float physics_min_camera_distance(float rs) { return 2.5f * rs; }

float physics_omega(float r, float rs) {
    return sqrtf(0.5f * rs / (r * r * r));
}
