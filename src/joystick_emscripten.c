#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "arc.h"
#include "joystick.h"
#include "plat_joystick.h"

int joysticks_present = 0;
joystick_t joystick_state[MAX_JOYSTICKS];

plat_joystick_t plat_joystick_state[MAX_PLAT_JOYSTICKS];

void joystick_init()
{
}

void joystick_close()
{
}

void joystick_poll_host()
{
}
