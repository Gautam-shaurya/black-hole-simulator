#include "errorlog.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef SHADER_SOURCE_DIR
#define SHADER_SOURCE_DIR "shaders"
#endif

/* The program may be started from the build folder, an IDE sub-folder
 * (Release/, Debug/) or the project root, so several locations are tried. */
static const char *k_search_dirs[] = {
    "shaders/", "../shaders/", "../../shaders/", "../../../shaders/", NULL
};

static int g_missing_file = 0;
int shader_last_failure_was_missing_file(void) { return g_missing_file; }

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 0) { fclose(f); return NULL; }
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t got = fread(buf, 1, (size_t)n, f);
    fclose(f);
    buf[got] = '\0';
    return buf;
}

static char *load_shader_text(const char *name) {
    char path[1024];
    for (int i = 0; k_search_dirs[i]; ++i) {
        snprintf(path, sizeof path, "%s%s", k_search_dirs[i], name);
        char *s = read_file(path);
        if (s) return s;
    }
    snprintf(path, sizeof path, "%s/%s", SHADER_SOURCE_DIR, name);
    return read_file(path);
}

static GLuint compile_stage(GLenum type, const char *file) {
    char *src = load_shader_text(file);
    if (!src) {
        g_missing_file = 1;
        bhs_log("Shader file not found: %s (the shaders folder must be next to the program - was the ZIP fully extracted?)", file);
        return 0;
    }
    GLuint sh = glCreateShader(type);
    const GLchar *srcs[1] = { src };
    glShaderSource(sh, 1, srcs, NULL);
    glCompileShader(sh);
    free(src);

    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        char *log = (char *)malloc((size_t)len + 1);
        if (log) {
            glGetShaderInfoLog(sh, len, NULL, log);
            log[len] = '\0';
            bhs_log("Shader compile error in %s:\n%s", file, log);
            free(log);
        }
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

GLuint shader_load_program(const char *vert_file, const char *frag_file) {
    GLuint vs = compile_stage(GL_VERTEX_SHADER, vert_file);
    GLuint fs = compile_stage(GL_FRAGMENT_SHADER, frag_file);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return 0;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        char *log = (char *)malloc((size_t)len + 1);
        if (log) {
            glGetProgramInfoLog(prog, len, NULL, log);
            log[len] = '\0';
            bhs_log("Program link error (%s + %s):\n%s", vert_file, frag_file, log);
            free(log);
        }
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

void shader_delete(GLuint program) {
    if (program) glDeleteProgram(program);
}
