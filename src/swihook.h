#ifndef SWIHOOK_H
#define SWIHOOK_H

#include <stdint.h>

/*SWIs the frontend watches (see swihook.c). Called from opSWI with the SWI
  number, X bit clear, before the SWI runs*/
extern int swihook_count;
void swihook_call(uint32_t swi, uint32_t *regs);

static inline void swihook_check(uint32_t swi, uint32_t *regs)
{
	if (swihook_count)
		swihook_call(swi, regs);
}

#endif
