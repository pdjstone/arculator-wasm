#include "arc.h"
#include "plat_input.h"

static int mouse_buttons;
static int mouse_x = 0, mouse_y = 0;

int mouse[3];

extern void browser_init_mouse(int *x, int *y, int *b, int *mouse_mode);

extern void notify_mouse_lock_required(int needed);

static void mouse_init()
{
	browser_init_mouse(&mouse_x, &mouse_y, &mouse_buttons, &mouse_mode);
}

static void mouse_close()
{
}

void mouse_capture_enable()
{

}

void mouse_capture_disable()
{

}

void mouse_poll_host()
{

}

void mouse_get_rel(int *x, int *y)
{
	*x = mouse_x;
	*y = mouse_y;
	mouse_x = mouse_y = 0;
}

void mouse_get_abs(int *x, int *y, int *b)
{
	*x = mouse_x;
	*y = mouse_y;
	*b = mouse_buttons;
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
