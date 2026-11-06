#include "accretion_disk.h"

void accretion_disk_upload(GLuint program, const SimParams *sim) {
    glUniform1f(glGetUniformLocation(program, "uDiskIn"), DISK_INNER_RS);
    glUniform1f(glGetUniformLocation(program, "uDiskOut"), DISK_OUTER_RS);
    glUniform1f(glGetUniformLocation(program, "uDiskBright"), sim->diskBrightness);
    glUniform1f(glGetUniformLocation(program, "uDiskSpeed"), sim->diskSpeed);
    /* The shader works in units of rs, so its clock is simulation time / rs. */
    glUniform1f(glGetUniformLocation(program, "uTime"), (float)(sim->simTime / sim->bhSize));
}

float accretion_disk_inner_omega(const SimParams *sim) {
    return physics_omega(DISK_INNER_RS * sim->bhSize, sim->bhSize) * sim->diskSpeed;
}
