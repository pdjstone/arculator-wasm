/*SWI hooks: lets the frontend watch for SWIs as the emulated CPU executes
  them, e.g. to see which music a program loads into a tracker module.

  The frontend sets a list of SWI numbers with arc_swi_hook_set(). When the
  CPU reaches a SWI instruction for one of them (with or without the X bit)
  the worker's self.arcSwiHook(swi, regs) is called synchronously, before the
  SWI runs, with a copy of r0-r15. It can read emulated memory with
  arc_mem_read(). Nothing here depends on RISC OS, so it works while a
  program has taken over the machine and turned interrupts off.

  The hook only sees the SWI being called, not its results.*/
#include <stdint.h>
#include <stdio.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif
#include "arc.h"
#include "swihook.h"

#define SWIHOOK_MAX      32
#define MEM_READ_MAX     65536

int swihook_count;
static uint32_t swihook_swis[SWIHOOK_MAX];
static uint8_t mem_read_buf[MEM_READ_MAX];

#ifdef __EMSCRIPTEN__
EM_JS(void, swihook_js_call, (uint32_t swi, const uint32_t *regs), {
	if (self.arcSwiHook) self.arcSwiHook(swi >>> 0, HEAPU32.slice(regs >> 2, (regs >> 2) + 16));
});
#else
static void swihook_js_call(uint32_t swi, const uint32_t *regs)
{
	printf("SWI &%X r0=%08X r1=%08X r2=%08X\n", swi, regs[0], regs[1], regs[2]);
}
#endif

void swihook_call(uint32_t swi, uint32_t *regs)
{
	for (int i = 0; i < swihook_count; i++)
	{
		if (swihook_swis[i] == swi)
		{
			swihook_js_call(swi, regs);
			return;
		}
	}
}

/*Sets the SWIs to watch, replacing the list. The X bit is ignored. Returns
  the number set, or -1 if there are too many*/
int EMSCRIPTEN_KEEPALIVE arc_swi_hook_set(const uint32_t *swis, int n)
{
	if (n < 0 || n > SWIHOOK_MAX)
		return -1;
	for (int i = 0; i < n; i++)
		swihook_swis[i] = swis[i] & ~0x20000;
	swihook_count = n;
	return n;
}

/*Copies len bytes from logical address addr in the current memory map into
  a buffer, ignoring access permissions and with no side effects (it stops
  at I/O). Returns the number of bytes copied, which is short if it reaches a
  page that isn't mapped. The bytes are at arc_mem_read_buffer()*/
int EMSCRIPTEN_KEEPALIVE arc_mem_read(uint32_t addr, int len)
{
	int i;

	if (len > MEM_READ_MAX)
		len = MEM_READ_MAX;
	for (i = 0; i < len; i++)
	{
		uint32_t a = (addr + i) & 0x3ffffff;

		/*memstat is 0 for pages that aren't mapped, and I/O*/
		if (!memstat[a >> 12] || !mempoint[a >> 12])
			break;
		mem_read_buf[i] = mempoint[a >> 12][a];
	}
	return i;
}

uint8_t * EMSCRIPTEN_KEEPALIVE arc_mem_read_buffer(void)
{
	return mem_read_buf;
}
