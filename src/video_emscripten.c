#include <string.h>
#include <emscripten.h>
#include <stdio.h>
#include "plat_video.h"
#include "vidc.h"


int selected_video_renderer;
int skip_video_render;


int video_renderer_init(void *main_window) {
    return 1;
}

int video_renderer_reinit(void *main_window) {
    return 1;
}

void video_renderer_close() {

}

void video_renderer_update(BITMAP *src, int x1, int y1, int x2, int y2, int dest_x, int dest_y) {

}

void video_renderer_present(int src_x, int src_y, int src_w, int src_h, int dblscan) {

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
