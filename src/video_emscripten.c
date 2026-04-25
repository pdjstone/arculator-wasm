#include <string.h>
#include <emscripten.h>
#include <stdio.h>
#include "plat_video.h"
#include "vidc.h"


int selected_video_renderer;
int skip_video_render;


extern void emscripten_render_update(void* src, int src_x, int src_y, int w, int h);
extern void emscripten_render_present(int src_x, int src_y, int src_w, int src_h, int dblscan);
extern void emscripten_update_window_size(int w, int h);

int video_renderer_reinit(void *main_window) {
    return 1;
}

void video_renderer_close() {

}

void updatewindowsize(int w, int h) {
    if (skip_video_render || w < 0 || h < 0)
        return;
    emscripten_update_window_size(w, h);
}

void video_renderer_update(BITMAP *src, int src_x, int src_y, int dest_x, int dest_y, int w, int h) {
    if (skip_video_render || w < 0 || h < 0)
        return;

    // dest_x and dest_y are always 0
    emscripten_render_update(src->dat, src_x, src_y, w, h);
}

void video_renderer_present(int src_x, int src_y, int src_w, int src_h, int dblscan) {
    if (skip_video_render || src_w < 0 || src_h < 0)
        return;
    emscripten_render_present(src_x, src_y, src_w, src_h, dblscan);
}

int video_renderer_available(int id) {
    return (id == RENDERER_OPENGL || id == RENDERER_AUTO);
}

char *video_renderer_get_name(int id) {

    if (id == RENDERER_AUTO) {
        return "auto";
    }
    if (id == RENDERER_OPENGL) {
        return "opengl";
    }
    return "unknown";
}

int video_renderer_get_id(char *name) {
    if (!strcmp(name, "auto"))
        return RENDERER_AUTO;
    if (!strcmp(name, "opengl"))
        return RENDERER_OPENGL;
    return 0;
}
