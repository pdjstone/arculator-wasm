void input_init();
void input_close();

void mouse_poll_host();
void mouse_get_rel(int *x, int *y);
void mouse_get_abs(int *x, int *y, int *b);
int mouse_get_buttons();
void mouse_capture_enable();
void mouse_capture_disable();
void notify_mouse_lock_required(int needed);

void keyboard_poll_host();
extern int key[512];

#ifndef __EMSCRIPTEN__
#include "keyboard_sdl.h"
#else
#include "keyboard_emscripten.h"
#endif