#include "starfield.h"

void starfield_upload(GLuint program, const SimParams *sim, float pixel_angle) {
    glUniform1f(glGetUniformLocation(program, "uStars"), sim->starfield ? 1.0f : 0.0f);
    glUniform1f(glGetUniformLocation(program, "uSkyTime"), (float)sim->simTime);
    glUniform1f(glGetUniformLocation(program, "uPixelAngle"), pixel_angle);
}
