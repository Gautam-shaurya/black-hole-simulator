#ifndef STARFIELD_H
#define STARFIELD_H
#include "gl_loader.h"
#include "physics.h"

/* The star field and nebula are procedural (evaluated in the black-hole
 * shader along each LENSED ray so the stars bend with the light).
 * This module uploads their uniforms. pixel_angle = angular size of a pixel. */
void starfield_upload(GLuint program, const SimParams *sim, float pixel_angle);
#endif
