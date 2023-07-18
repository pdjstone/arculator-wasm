/*Arculator 2.1 by Sarah Walker
  Main header file*/

//#define printf rpclog

#define VERSION_STRING "v2.1"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

#define ignore_result(r) if ((r)) {  };
#include "debugger.h"

/*Misc*/
extern void rpclog(const char *format, ...);
extern void error(const char *format, ...);
extern void fatal(const char *format, ...);
void arc_print_error(const char *format, ...);

#ifdef DEBUG_CMOS
#define LOG_CMOS rpclog
#else
#define LOG_CMOS(...) ((void)0)
#endif

#ifdef DEBUG_EVENT_LOOP
#define LOG_EVENT_LOOP rpclog
#else
#define LOG_EVENT_LOOP(...) ((void)0)
#endif

#ifdef DEBUG_KB_MOUSE
#define LOG_KB_MOUSE rpclog
#else
#define LOG_KB_MOUSE(...) ((void)0)
#endif

#ifdef DEBUG_MEMC_VIDEO
#define LOG_MEMC_VIDEO rpclog
#else
#define LOG_MEMC_VIDEO(...) ((void)0)
#endif

#ifdef DEBUG_VIDC_REGISTERS
#define LOG_VIDC_REGISTERS rpclog
#else
#define LOG_VIDC_REGISTERS(...) ((void)0)
#endif

#ifdef DEBUG_VIDC_TIMING
#define LOG_VIDC_TIMING rpclog
#else
#define LOG_VIDC_TIMING(...) ((void)0)
#endif

#ifdef DEBUG_VIDEO_FRAMES
#define LOG_VIDEO_FRAMES rpclog
#else
#define LOG_VIDEO_FRAMES(...) ((void)0)
#endif

#ifdef DEBUG_DATABORT
#define LOG_DATABORT(...) do { rpclog(__VA_ARGS__); dumpregs(); } while (0)
#else
#define LOG_DATABORT(...) ((void)0)
#endif


#define container_of(ptr, type, member) ({                      \
	const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
	(type *)( (char *)__mptr - offsetof(type,member) );})


#define nr_elems(array) (int)(sizeof(array) / sizeof(array[0]))

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

extern void arc_set_cpu(int cpu, int memc);
extern void updatewindowsize(int x, int y);

extern int updatemips,inssec;
extern float inssecf;

/*ARM*/
extern uint32_t armregs[16];
extern int armirq,armfiq;
#define PC ((armregs[15])&0x3FFFFFC)
extern int ins,output;
extern int inscount;

extern void resetarm();
extern void execarm(int cycs);
extern void dumpregs();
extern int databort;
extern uint32_t opcode;

extern int fpaena;
extern int fpaopcode(uint32_t opcode);
extern void resetfpa();

/*CP15*/
extern void resetcp15();
extern uint32_t readcp15(int reg);
extern void writecp15(int reg, uint32_t val);

/*Memory*/
extern const int modepritabler[3][8],modepritablew[3][8];
extern uint8_t *mempoint[0x4000];
extern uint8_t memstat[0x4000];
extern uint32_t *ram,*rom;

enum {
	MEMMODE_USER,
	MEMMODE_OS,
	MEMMODE_SUPER
};
extern int memmode;

extern void initmem(int memsize);
extern void resizemem(int memsize);
extern int loadrom();
extern void resetpagesize(int pagesize);

#define readmemb(a)    ((modepritabler[memmode][memstat[((a) >> 12) & 0x3FFF]]) ? mempoint[((a) >> 12) & 0x3FFF][(a)] : readmemfb(a))
#define readmeml(a)    ((modepritabler[memmode][memstat[((a) >> 12) & 0x3FFF]]) ? *(uint32_t *)&mempoint[((a) >> 12) & 0x3FFF][(a) & ~3] : readmemfl(a))
#define readmemff(a)    ((modepritabler[memmode][memstat[((a) >> 12) & 0x3FFF]]) ? *(uint32_t *)&mempoint[((a) >> 12) & 0x3FFF][(a) & ~3] : readmemf(a))

extern uint32_t readmemf(uint32_t a);
extern uint8_t readmemfb(uint32_t a);
extern uint32_t readmemfl(uint32_t a);
extern void writememfb(uint32_t a,uint8_t v);
extern void writememfl(uint32_t a,uint32_t v);

static inline void writememb(uint32_t a, uint8_t v)
{
	if (debugon)
		debug_writememb(a, v);

	if (modepritablew[memmode][memstat[((a) >> 12) & 0x3FFF]])
		mempoint[((a) >> 12) & 0x3FFF][(a)] = v;
	else
		writememfb(a, v);
}

static inline void writememl(uint32_t a, uint32_t v)
{
	if (debugon)
		debug_writememl(a, v);

	if (modepritablew[memmode][memstat[((a) >> 12) & 0x3FFF]])
		*(uint32_t *)&mempoint[((a) >> 12) & 0x3FFF][(a) & ~3] = v;
	else
		writememfl(a, v);
}

/*MEMC*/
extern uint32_t vinit;
extern uint32_t vstart; /*Start of video RAM*/
extern uint32_t vend; /*End of video RAM*/
extern uint32_t cinit;
extern int osmode;
extern int prefabort, prefabort_next;

extern void initmemc();
extern void writememc(uint32_t a);
extern void writecam(uint32_t a);

/*VIDC*/
extern int soundper;
extern int offsetx,offsety;
extern int fullscreen;
extern int fullborders,noborders;
extern int dblscan;
extern int stereoimages[8];
extern int flyback;
extern int skip_video_render;

extern void initvid();
extern void reinitvideo();
extern void writevidc(uint32_t v);
extern int vidcgetoverflow();
extern void pollline();
extern void setredrawall();
extern void clearbitmap();

/*Disc*/
extern char discname[4][512];
extern int discchange[4];
extern int fdctype;
extern int readflash[4];

/* normal mouse input using relative movements send via keyboard controller */
#define MOUSE_MODE_RELATIVE 1

/* absolute mouse coordinates poked directly into OS memory, and mouse SWIs intercepted */
#define MOUSE_MODE_ABSOLUTE 2

extern int mouse_mode;

extern char exname[512];

/*Eterna*/
extern uint8_t readeterna(uint32_t addr);
extern void writeeterna(uint32_t addr, uint32_t val);

/*Config*/
extern int romset;
extern int stereo;


extern int soundena;

extern int fullscreen;
extern int fullborders,noborders;
extern int firstfull;
extern int dblscan;

extern int arm3;

extern int memsize;

extern int fdctype;


extern int speed_mhz;


extern int mousecapture;
extern int fast_forward_to_time_ms;
extern long long total_emulation_millis;

void updateins();

void get_executable_name(char *s, int size);


void updatewindowsize(int x, int y);

int arc_init();
void arc_reset();
void arc_run(int millisecs);
void arc_close();


void arc_start_main_thread(void *wx_window, void *wx_menu);
void arc_stop_main_thread();
void arc_pause_main_thread();
void arc_resume_main_thread();

void arc_do_reset();
void arc_disc_change(int drive, char *fn);
void arc_disc_eject(int drive);
void arc_enter_fullscreen();
void arc_renderer_reset();
void arc_set_display_mode(int new_display_mode);
void arc_set_dblscan(int new_dblscan);


void arc_stop_emulation();
void arc_popup_menu();
void arc_update_menu();
void *wx_getnativemenu(void *menu);
