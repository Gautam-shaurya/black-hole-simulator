#ifndef BLACKHOLE_H
#define BLACKHOLE_H
#include "gl_loader.h"
#include "camera.h"
#include "physics.h"

typedef struct BlackHole {
    GLuint program;
    GLuint vao;  /* empty VAO, core profile requires one for gl_VertexID draws */
} BlackHole;

int  blackhole_init(BlackHole *bh);
void blackhole_render(const BlackHole *bh, const Camera *cam, const SimParams *sim, int width, int height);
void blackhole_shutdown(BlackHole *bh);
#endif
