#ifndef SHADER_H
#define SHADER_H
#include "gl_loader.h"

/* Loads, compiles and links a vertex + fragment shader pair from the
 * shaders/ folder. Returns the program id, or 0 on failure (the compiler /
 * linker log is printed to stderr with the file name). */
GLuint shader_load_program(const char *vert_file, const char *frag_file);

/* 1 if the most recent failure was a shader FILE that could not be found. */
int shader_last_failure_was_missing_file(void);

/* Deletes a program (safe on 0). */
void shader_delete(GLuint program);
#endif
