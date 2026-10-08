/*Arculator 2.1 by Sarah Walker
  Generic SDL-based main window handling*/
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "arc.h"
#include "config.h"
#include "disc.h"
#include "ioc.h"
#include "sound.h"
#include "plat_sound.h"
#include "plat_input.h"
#include "plat_video.h"
#include "vidc.h"
#include "video.h"
#include "video_sdl2.h"

#ifndef __EMSCRIPTEN__
#define EMSCRIPTEN_KEEPALIVE
#endif

static int winsizex = 0, winsizey = 0;
static int win_doresize = 0;
static int win_renderer_reset = 0;

#define MAX_TICKS_PER_FRAME 500

static int fixed_fps = 0;

void updatewindowsize(int x, int y)
{
        winsizex = x; winsizey = y;
        win_doresize = 1;
}

static volatile int pause_main_thread = 0;

static SDL_mutex *main_thread_mutex = NULL;


void arcloop()
{
        /*Resize window to match screen mode*/
        if (win_doresize && fast_forward_to_time_ms == 0)
        {
                SDL_Rect rect;

                win_doresize = 0;

                SDL_GetWindowSize(sdl_main_window, &rect.w, &rect.h);
                if (rect.w != winsizex || rect.h != winsizey)
                {
                        rpclog("Resizing window to %d, %d\n", winsizex, winsizey);
                        SDL_GetWindowPosition(sdl_main_window, &rect.x, &rect.y);
                        SDL_SetWindowSize(sdl_main_window, winsizex, winsizey);
                        SDL_SetWindowPosition(sdl_main_window, rect.x, rect.y);
                }
        }

        if (win_renderer_reset)
        {
                win_renderer_reset = 0;

                if (!video_renderer_reinit(NULL))
                        fatal("Video renderer init failed");
        }

        static Uint32 last_timer_ticks = 0;
        Uint32 current_timer_ticks = SDL_GetTicks();
        Uint32 ticks_since_last = current_timer_ticks - last_timer_ticks;
        last_timer_ticks = current_timer_ticks;

        int run_ms = 0;
        if (fast_forward_to_time_ms != 0 && total_emulation_millis < fast_forward_to_time_ms) {
                run_ms = MAX_TICKS_PER_FRAME;
                sound_enable(0);
                //rpclog("fast forward to %d %d\n", fast_forward_to_time_ms, total_emulation_millis);
        } else {
                if (fast_forward_to_time_ms != 0) {
                        rpclog("finished fast forward - re-enabling sound and video\n");
                        fast_forward_to_time_ms = 0;
                        sound_enable(1);
                        skip_video_render = 0;
                        win_doresize = 1;
                }
                run_ms = ticks_since_last < MAX_TICKS_PER_FRAME ? ticks_since_last : MAX_TICKS_PER_FRAME;
        }

        SDL_LockMutex(main_thread_mutex);

        if (!pause_main_thread)
                arc_run(run_ms);

        SDL_UnlockMutex(main_thread_mutex);
       

}

static int arc_main_thread()
{
        rpclog("Arculator startup\n");

        arc_init();

        if (!video_renderer_init(NULL))
        {
                fatal("Video renderer init failed");
        }
        input_init();

        // if fixed_fps is 0, emscripten will use requestAnimationFrame
        emscripten_set_main_loop(arcloop, fixed_fps, 1);
      
        return 0;
}


void EMSCRIPTEN_KEEPALIVE arc_pause_main_thread()
{
        SDL_LockMutex(main_thread_mutex);
        pause_main_thread = 1;
        SDL_UnlockMutex(main_thread_mutex);
}

long long EMSCRIPTEN_KEEPALIVE arc_get_emulation_ms()
{
        return total_emulation_millis;
}

void EMSCRIPTEN_KEEPALIVE arc_resume_main_thread()
{
        SDL_LockMutex(main_thread_mutex);
        pause_main_thread = 0;
        SDL_UnlockMutex(main_thread_mutex);
}


void EMSCRIPTEN_KEEPALIVE arc_do_reset()
{
        SDL_LockMutex(main_thread_mutex);
        arc_reset();
        SDL_UnlockMutex(main_thread_mutex);
}

void EMSCRIPTEN_KEEPALIVE arc_load_config_and_reset(char *config_name)
{
        SDL_LockMutex(main_thread_mutex);
        arc_close();
        snprintf(machine_config_file, 256, "configs/%s.cfg", config_name);
        strncpy(machine_config_name, config_name, 255);
        rpclog("arc_load_config_and_reset: machine_config_name=%s machine_config_file=%s\n", machine_config_name, machine_config_file);
        loadconfig();
        arc_init();
        SDL_UnlockMutex(main_thread_mutex);
}

void EMSCRIPTEN_KEEPALIVE arc_disc_change(int drive, char *fn)
{
        rpclog("arc_disc_change: drive=%i fn=%s\n", drive, fn);

        SDL_LockMutex(main_thread_mutex);

        disc_close(drive);
        strcpy(discname[drive], fn);
        disc_load(drive, discname[drive]);
        ioc_discchange(drive);

        SDL_UnlockMutex(main_thread_mutex);
}

void EMSCRIPTEN_KEEPALIVE arc_disc_eject(int drive)
{
        rpclog("arc_disc_eject: drive=%i\n", drive);

        SDL_LockMutex(main_thread_mutex);

        ioc_discchange(drive);
        disc_close(drive);
        discname[drive][0] = 0;

        SDL_UnlockMutex(main_thread_mutex);
}

void EMSCRIPTEN_KEEPALIVE arc_fast_forward(int time_ms)
{
        soundena = 0;
        skip_video_render = 1;
        fast_forward_to_time_ms = time_ms;
        rpclog("arc_fast_forward: %d\n", time_ms);
}

/*Dump stereo audio to a PCM WAV file in the Emscripten filesystem, starting
  after 'delay' seconds of emulation and lasting 'duration' seconds*/
void EMSCRIPTEN_KEEPALIVE arc_dump_audio(double delay, double duration)
{
        rpclog("arc_dump_audio: delay=%f duration=%f\n", delay, duration);
        sound_dump_audio(delay, duration);
}

/*Current audio dump state: one of AUDIO_DUMP_OFF/WAITING/RECORDING/FINISHED
  from sound.h*/
int EMSCRIPTEN_KEEPALIVE arc_get_audio_dump_state()
{
        return sound_get_dump_state();
}

void arc_stop_main_thread()
{
        //SDL_WaitThread(main_thread, NULL);
        SDL_DestroyMutex(main_thread_mutex);
        main_thread_mutex = NULL;
}

void EMSCRIPTEN_KEEPALIVE arc_renderer_reset()
{
        win_renderer_reset = 1;
}

void EMSCRIPTEN_KEEPALIVE arc_set_display_mode(int new_display_mode)
{
        SDL_LockMutex(main_thread_mutex);

        display_mode = new_display_mode;
        clearbitmap();
        setredrawall();

        SDL_UnlockMutex(main_thread_mutex);
}

void EMSCRIPTEN_KEEPALIVE arc_set_dblscan(int new_dblscan)
{
        SDL_LockMutex(main_thread_mutex);

        dblscan = new_dblscan;
        clearbitmap();

        SDL_UnlockMutex(main_thread_mutex);
}


void EMSCRIPTEN_KEEPALIVE arc_set_sound_filter(int filter)
{
        SDL_LockMutex(main_thread_mutex);
        if (filter >=0 && filter <= 2) {
                sound_filter = filter;
                sound_update_filter();
        }
        SDL_UnlockMutex(main_thread_mutex);

}

void EMSCRIPTEN_KEEPALIVE arc_enable_sound(int enable) 
{
        sound_enable(enable);
}

int EMSCRIPTEN_KEEPALIVE main(int argc, char** argv)
{
        rpclog("emscripten main - argc=%d\n", argc);
        if (argc > 1)
        {
                fixed_fps = atoi(argv[1]);
                rpclog("setting fixed_fps=%d\n", fixed_fps);
        }
        if (argc > 2)
        {
                snprintf(machine_config_file, 255, "configs/%s.cfg", argv[2]);
                strncpy(machine_config_name, argv[2], 255);
                rpclog("machine_config_name=%s machine_config_file=%s\n", machine_config_name, machine_config_file);
        }
        main_thread_mutex = SDL_CreateMutex();
        arc_main_thread();
}

void arc_print_error(const char *format, ...) {

}