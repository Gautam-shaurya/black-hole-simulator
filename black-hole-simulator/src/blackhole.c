#include "blackhole.h"
#include "shader.h"
#include "accretion_disk.h"
#include "starfield.h"
#include <math.h>
#include <stdio.h>

int blackhole_init(BlackHole *bh) {
    bh->program = shader_load_program("fullscreen.vert", "blackhole.frag");
    if (!bh->program) return 0;
    glGenVertexArrays(1, &bh->vao);
    return 1;
}

void blackhole_render(const BlackHole *bh, const Camera *cam, const SimParams *sim, int width, int height) {
    GLuint p = bh->program;
    float tan_half = tanf(cam->fovY * 0.5f);

    glUseProgram(p);
    glUniform2f(glGetUniformLocation(p, "uRes"), (float)width, (float)height);
    glUniform3f(glGetUniformLocation(p, "uCamPos"), cam->pos.x, cam->pos.y, cam->pos.z);
    glUniform3f(glGetUniformLocation(p, "uCamRight"), cam->right.x, cam->right.y, cam->right.z);
    glUniform3f(glGetUniformLocation(p, "uCamUp"), cam->up.x, cam->up.y, cam->up.z);
    glUniform3f(glGetUniformLocation(p, "uCamFwd"), cam->fwd.x, cam->fwd.y, cam->fwd.z);
    glUniform1f(glGetUniformLocation(p, "uTanHalfFov"), tan_half);
    glUniform1f(glGetUniformLocation(p, "uRs"), sim->bhSize);
    /* lensing off => straight rays (the dark horizon disc is still drawn) */
    glUniform1f(glGetUniformLocation(p, "uLens"), sim->lensingOn ? sim->lensing : 0.0f);

    accretion_disk_upload(p, sim);
    starfield_upload(p, sim, 2.0f * tan_half / (float)height);

    glBindVertexArray(bh->vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void blackhole_shutdown(BlackHole *bh) {
    shader_delete(bh->program);
    if (bh->vao) glDeleteVertexArrays(1, &bh->vao);
    bh->program = 0; bh->vao = 0;
}
