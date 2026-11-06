#include "errorlog.h"
#include "postfx.h"
#include "shader.h"
#include <stdio.h>
#include <string.h>

static int make_target(GLuint *fbo, GLuint *tex, int w, int h) {
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    int ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return ok;
}

static void free_targets(PostFX *fx) {
    if (fx->sceneFbo) glDeleteFramebuffers(1, &fx->sceneFbo);
    if (fx->sceneTex) glDeleteTextures(1, &fx->sceneTex);
    fx->sceneFbo = fx->sceneTex = 0;
    for (int i = 0; i < BLOOM_LEVELS; ++i) {
        if (fx->fboA[i]) glDeleteFramebuffers(1, &fx->fboA[i]);
        if (fx->fboB[i]) glDeleteFramebuffers(1, &fx->fboB[i]);
        if (fx->texA[i]) glDeleteTextures(1, &fx->texA[i]);
        if (fx->texB[i]) glDeleteTextures(1, &fx->texB[i]);
        fx->fboA[i] = fx->fboB[i] = fx->texA[i] = fx->texB[i] = 0;
    }
}

static int build_targets(PostFX *fx, int w, int h) {
    fx->width = w; fx->height = h;
    int ok = make_target(&fx->sceneFbo, &fx->sceneTex, w, h);
    for (int i = 0; i < BLOOM_LEVELS; ++i) {
        int lw = w >> (i + 1), lh = h >> (i + 1);
        fx->lw[i] = lw < 1 ? 1 : lw;
        fx->lh[i] = lh < 1 ? 1 : lh;
        ok &= make_target(&fx->fboA[i], &fx->texA[i], fx->lw[i], fx->lh[i]);
        ok &= make_target(&fx->fboB[i], &fx->texB[i], fx->lw[i], fx->lh[i]);
    }
    return ok;
}

int postfx_init(PostFX *fx, int width, int height) {
    memset(fx, 0, sizeof *fx);
    fx->progDown  = shader_load_program("fullscreen.vert", "post_down.frag");
    fx->progBlur  = shader_load_program("fullscreen.vert", "post_blur.frag");
    fx->progFinal = shader_load_program("fullscreen.vert", "post_final.frag");
    if (!fx->progDown || !fx->progBlur || !fx->progFinal) return 0;
    glGenVertexArrays(1, &fx->vao);
    if (!build_targets(fx, width, height)) {
        bhs_log("Framebuffer incomplete (HDR float render targets unsupported?)");
        return 0;
    }
    return 1;
}

int postfx_resize(PostFX *fx, int width, int height) {
    if (width == fx->width && height == fx->height) return 1;
    free_targets(fx);
    return build_targets(fx, width, height);
}

void postfx_begin_scene(PostFX *fx) {
    glBindFramebuffer(GL_FRAMEBUFFER, fx->sceneFbo);
    glViewport(0, 0, fx->width, fx->height);
}

static void bind_tex(int unit, GLuint tex) {
    glActiveTexture(GL_TEXTURE0 + (GLuint)unit);
    glBindTexture(GL_TEXTURE_2D, tex);
}

void postfx_present(PostFX *fx, const PostParams *pp) {
    glBindVertexArray(fx->vao);

    /* ---- bright pass + downsample chain, each level blurred H then V ---- */
    for (int i = 0; i < BLOOM_LEVELS; ++i) {
        GLuint src = i == 0 ? fx->sceneTex : fx->texA[i - 1];
        int sw = i == 0 ? fx->width : fx->lw[i - 1];
        int sh = i == 0 ? fx->height : fx->lh[i - 1];

        glBindFramebuffer(GL_FRAMEBUFFER, fx->fboA[i]);
        glViewport(0, 0, fx->lw[i], fx->lh[i]);
        glUseProgram(fx->progDown);
        bind_tex(0, src);
        glUniform1i(glGetUniformLocation(fx->progDown, "uTex"), 0);
        glUniform2f(glGetUniformLocation(fx->progDown, "uSrcTexel"), 1.0f / (float)sw, 1.0f / (float)sh);
        glUniform2f(glGetUniformLocation(fx->progDown, "uDstSize"), (float)fx->lw[i], (float)fx->lh[i]);
        glUniform1f(glGetUniformLocation(fx->progDown, "uThreshold"), i == 0 ? 1.0f : 0.0f);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glUseProgram(fx->progBlur);
        glUniform1i(glGetUniformLocation(fx->progBlur, "uTex"), 0);
        glUniform2f(glGetUniformLocation(fx->progBlur, "uDstSize"), (float)fx->lw[i], (float)fx->lh[i]);

        glBindFramebuffer(GL_FRAMEBUFFER, fx->fboB[i]);
        bind_tex(0, fx->texA[i]);
        glUniform2f(glGetUniformLocation(fx->progBlur, "uDir"), 1.0f / (float)fx->lw[i], 0.0f);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindFramebuffer(GL_FRAMEBUFFER, fx->fboA[i]);
        bind_tex(0, fx->texB[i]);
        glUniform2f(glGetUniformLocation(fx->progBlur, "uDir"), 0.0f, 1.0f / (float)fx->lh[i]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    /* ---- final composite to the window ---- */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fx->width, fx->height);
    glUseProgram(fx->progFinal);
    bind_tex(0, fx->sceneTex);
    glUniform1i(glGetUniformLocation(fx->progFinal, "uScene"), 0);
    static const char *names[BLOOM_LEVELS] = { "uBloom0", "uBloom1", "uBloom2", "uBloom3", "uBloom4" };
    for (int i = 0; i < BLOOM_LEVELS; ++i) {
        bind_tex(1 + i, fx->texA[i]);
        glUniform1i(glGetUniformLocation(fx->progFinal, names[i]), 1 + i);
    }
    glUniform2f(glGetUniformLocation(fx->progFinal, "uRes"), (float)fx->width, (float)fx->height);
    glUniform1f(glGetUniformLocation(fx->progFinal, "uBloomStrength"), pp->bloomStrength);
    glUniform1f(glGetUniformLocation(fx->progFinal, "uExposure"), pp->exposure);
    glUniform1f(glGetUniformLocation(fx->progFinal, "uVignette"), pp->vignette);
    glUniform1f(glGetUniformLocation(fx->progFinal, "uCinematic"), pp->cinematic);
    glUniform1f(glGetUniformLocation(fx->progFinal, "uLetterbox"), pp->letterbox);
    glUniform1f(glGetUniformLocation(fx->progFinal, "uTime"), pp->time);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(0);
}

void postfx_shutdown(PostFX *fx) {
    free_targets(fx);
    shader_delete(fx->progDown);
    shader_delete(fx->progBlur);
    shader_delete(fx->progFinal);
    if (fx->vao) glDeleteVertexArrays(1, &fx->vao);
    memset(fx, 0, sizeof *fx);
}
