#ifndef POSTFX_H
#define POSTFX_H
#include "gl_loader.h"

#define BLOOM_LEVELS 5

typedef struct PostParams {
    float bloomStrength, exposure, vignette, cinematic, letterbox, time;
} PostParams;

typedef struct PostFX {
    int width, height;
    GLuint sceneFbo, sceneTex;              /* HDR (RGBA16F) scene target */
    GLuint fboA[BLOOM_LEVELS], texA[BLOOM_LEVELS];
    GLuint fboB[BLOOM_LEVELS], texB[BLOOM_LEVELS];
    int lw[BLOOM_LEVELS], lh[BLOOM_LEVELS];
    GLuint progDown, progBlur, progFinal, vao;
} PostFX;

int  postfx_init(PostFX *fx, int width, int height);
int  postfx_resize(PostFX *fx, int width, int height);
void postfx_begin_scene(PostFX *fx);                          /* bind HDR target   */
void postfx_present(PostFX *fx, const PostParams *pp);        /* bloom + tonemap -> screen */
void postfx_shutdown(PostFX *fx);
#endif
