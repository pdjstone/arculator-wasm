/*Arculator 2.2 by Sarah Walker
  SDL2 input handling*/
#include <SDL.h>
#include <string.h>
#include "arc.h"
#include "plat_input.h"

static int mouse_buttons;
static int mouse_x = 0, mouse_y = 0;

static int mouse_capture = 0;

int mouse[3];

static void mouse_init()
{
}

static void mouse_close()
{
}

void mouse_capture_enable()
{
	mouse_capture = 1;
}

void mouse_capture_disable()
{
	mouse_capture = 0;
}

void mouse_poll_host()
{
	get_mouse_deltas(&mouse[0], &mouse[1], &mouse_buttons);
	mouse_x += mouse[0];
	mouse_y += mouse[1];
	//mouse_buttons = 0;

	// printf("mouse %d, %d\n", mouse_x, mouse_y);
}

void mouse_get_mickeys(int *x, int *y)
{
	*x = mouse_x;
	*y = mouse_y;
	mouse_x = mouse_y = 0;
}

int mouse_get_buttons()
{
	return mouse_buttons;
}

int key[512];

static void keyboard_init()
{
}

static void keyboard_close()
{
}

void keyboard_poll_host()
{
}

void input_init()
{
	mouse_init();
	keyboard_init();
}
void input_close()
{
	keyboard_close();
	mouse_close();
}
