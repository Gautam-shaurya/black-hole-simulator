#ifndef PARTICLES_H
#define PARTICLES_H
#include "gl_loader.h"
#include "camera.h"
#include "physics.h"

#define PARTICLE_CAPACITY 12000

typedef struct Particle {
    float r;      /* orbital radius in units of rs                      */
    float phi;    /* orbital angle (rad)                                */
    float yrel;   /* height above the disk plane / r                    */
    float inward; /* >0: spirals toward the hole                        */
    float temp;   /* random heat tag 0..1 (colour)                      */
    float size;   /* world size in units of rs                          */
    float phase;  /* brightness-pulsing phase                           */
    float speed;  /* personal orbital speed multiplier                  */
} Particle;

typedef struct ParticleSystem {
    Particle *p;
    float *verts;          /* CPU staging buffer: heads, then trail line pairs */
    int active;            /* number rendered this frame                       */
    unsigned int rng;
    float clock;
    GLuint vao, vbo, program;
} ParticleSystem;

int  particles_init(ParticleSystem *ps, const SimParams *sim);
void particles_reset(ParticleSystem *ps, const SimParams *sim);
void particles_update(ParticleSystem *ps, float sim_dt, const SimParams *sim);
void particles_render(ParticleSystem *ps, const Camera *cam, const SimParams *sim, int width, int height);
int  particles_active_count(const SimParams *sim);
void particles_shutdown(ParticleSystem *ps);
#endif
