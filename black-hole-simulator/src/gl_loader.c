#include "errorlog.h"
#include "gl_loader.h"
#include <stdio.h>

#define X(ret, name, args) ret (GLAPI_CALL *name) args = NULL;
GL_FUNCTION_LIST(X)
#undef X

int gl_load(void *(*get_proc)(const char *name)) {
    int missing = 0;
#define X(ret, name, args)                                              \
    *(void **)(&name) = get_proc(#name);                                \
    if (!name) { bhs_log("OpenGL function missing: %s", #name); ++missing; }
    GL_FUNCTION_LIST(X)
#undef X
    return missing;
}
