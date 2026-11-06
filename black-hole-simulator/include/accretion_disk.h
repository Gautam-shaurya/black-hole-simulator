#ifndef ACCRETION_DISK_H
#define ACCRETION_DISK_H
#include "gl_loader.h"
#include "physics.h"

/* Uploads the accretion-disk uniforms (radii, brightness, rotation clock) to
 * the black-hole shader. The disk itself is drawn procedurally in
 * shaders/blackhole.frag, so this module only owns its parameters. */
void accretion_disk_upload(GLuint program, const SimParams *sim);

/* Orbital angular velocity at the inner edge (rad per SU), including the user
 * disk-speed multiplier. Used by the information panel. */
float accretion_disk_inner_omega(const SimParams *sim);
#endif
