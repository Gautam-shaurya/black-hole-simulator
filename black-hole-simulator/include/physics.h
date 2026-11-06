/* physics.h - simulation parameters and the (simplified) physics formulas.
 *
 * ALL lengths are in "simulation units" (SU). The black hole is described by
 * its Schwarzschild radius rs (= event-horizon radius). Everything else is a
 * multiple of rs, exactly like real Schwarzschild geometry, but the numbers
 * are NOT astronomical measurements.
 */
#ifndef PHYSICS_H
#define PHYSICS_H

#define DISK_INNER_RS 3.0f   /* innermost stable circular orbit = 3 rs      */
#define DISK_OUTER_RS 11.0f  /* visual outer edge of the accretion disk      */
#define SIM_RATE 4.0f        /* simulation time units advanced per real second at 1x */

typedef struct SimParams {
    int paused;
    float timeScale;        /* simulation speed multiplier        */
    float diskBrightness;   /* accretion-disk brightness          */
    float particleDensity;  /* 0..1 fraction of the particle pool */
    float lensing;          /* lensing intensity (1 = Schwarzschild) */
    float bhSize;           /* Schwarzschild radius rs (SU)       */
    float diskSpeed;        /* disk rotation speed multiplier     */
    int starfield;
    int particles;
    int lensingOn;
    int cinematic;
    double simTime;         /* simulation clock (SU)              */
} SimParams;

void sim_defaults(SimParams *s);

/* Simulation time step for a real frame time (0 while paused). */
float sim_step(const SimParams *s, float real_dt);

float physics_photon_sphere(float rs);       /* 1.5 rs                     */
float physics_shadow_radius(float rs);       /* 3*sqrt(3)/2 rs ~ 2.598 rs  */
float physics_isco(float rs);                /* 3 rs                       */
float physics_min_camera_distance(float rs); /* keeps camera outside horizon */

/* Keplerian angular velocity (rad / SU) at radius r (SU) around a hole of
 * Schwarzschild radius rs:  omega = sqrt(G M / r^3), with G M = rs / 2 (c = 1). */
float physics_omega(float r, float rs);

#endif
