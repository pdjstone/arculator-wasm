#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif
#include "arc.h"
#include "config.h"
#include "disc.h"
#include "ioc.h"
#include "sound.h"
#include "sound_ring.h"
#include "plat_sound.h"
#include "plat_input.h"
#include "plat_video.h"
#include "vidc.h"
#include "video.h"


#ifndef __EMSCRIPTEN__
#define EMSCRIPTEN_KEEPALIVE
#endif

#define MAX_TICKS_PER_FRAME 500

static int fixed_fps = 0;

/*The page can't be seen, so there's no point drawing anything. Like a
  fast-forward this uses skip_video_render, and it's still set when one ends*/
static int page_hidden = 0;

/*Whether the user wants sound on. sound_enable() is also used to switch sound
  off during a fast-forward, which mustn't override this when it finishes.
  Starts off as the config file's sound_enable, which loadconfig() applies*/
static int user_sound_enabled = 1;



static volatile int pause_main_thread = 0;

/*Emulated time when the current fast-forward started, for its progress*/
static long long fast_forward_start_ms = 0;

/*Tells the worker how far a fast-forward has got (fast-forward.js), when it
  starts, as it runs and when it ends (active=0)*/
EM_JS(void, fast_forward_js_progress, (double start_ms, double now_ms, double target_ms, int active), {
        if (self.arcFastForwardProgress) self.arcFastForwardProgress(start_ms, now_ms, target_ms, !!active);
});

static void end_fast_forward()
{
        long long target_ms = fast_forward_to_time_ms;

        fast_forward_to_time_ms = 0;
        sound_enable(user_sound_enabled);
        skip_video_render = page_hidden;
        fast_forward_js_progress(fast_forward_start_ms, total_emulation_millis, target_ms, 0);
}


void arcloop()
{
        static double last_timer_ticks = 0;
        double current_timer_ticks = emscripten_performance_now();
        double ticks_since_last = current_timer_ticks - last_timer_ticks;
        last_timer_ticks = current_timer_ticks;

        int run_ms = 0;
        if (fast_forward_to_time_ms != 0 && total_emulation_millis < fast_forward_to_time_ms) {
                run_ms = MAX_TICKS_PER_FRAME;
                sound_enable(0);
                fast_forward_js_progress(fast_forward_start_ms, total_emulation_millis, fast_forward_to_time_ms, 1);
        } else {
                if (fast_forward_to_time_ms != 0) {
                        rpclog("finished fast forward - re-enabling sound and video\n");
                        end_fast_forward();
                }
                run_ms = ticks_since_last < MAX_TICKS_PER_FRAME ? ticks_since_last : MAX_TICKS_PER_FRAME;
                /*While the audio device is playing it is the master clock*/
                run_ms = sound_dev_pace_ms(run_ms);
        }

        if (!pause_main_thread) {
                int before = emscripten_performance_now();
                arc_run(run_ms);
                int emu_duration = emscripten_performance_now() - before;
                sound_stream_flush();
                sound_dev_note_loop(ticks_since_last, emscripten_performance_now() - current_timer_ticks);
                if (emu_duration >= run_ms) {
                        rpclog("arc_run ms=%d took %d !!\n", run_ms, emu_duration);
                }
        }

}

static int arc_main_thread()
{
        rpclog("Arculator startup\n");

        arc_init();
        user_sound_enabled = soundena;

        if (!video_renderer_init(NULL))
        {
                fatal("Video renderer init failed");
        }
        input_init();
        // if fixed_fps is 0, emscripten will use requestAnimationFrame
        emscripten_set_main_loop(arcloop, 60, 0);
        //emscripten_set_timeout_loop(arcloop, 20, 0);
        return 0;
}


void EMSCRIPTEN_KEEPALIVE arc_pause_main_thread()
{
        pause_main_thread = 1;
}

long long EMSCRIPTEN_KEEPALIVE arc_get_emulation_ms()
{
        return total_emulation_millis;
}

void EMSCRIPTEN_KEEPALIVE arc_resume_main_thread()
{
        pause_main_thread = 0;
}


/*A reset restarts emulated time, so it ends any fast-forward. Callers that
  want one after the reset start it again*/
void EMSCRIPTEN_KEEPALIVE arc_do_reset()
{
        if (fast_forward_to_time_ms != 0)
                end_fast_forward();
        arc_reset();
}

void EMSCRIPTEN_KEEPALIVE arc_load_config_and_reset(char *config_name)
{
        if (fast_forward_to_time_ms != 0)
                end_fast_forward();
        arc_close();
        snprintf(machine_config_file, 256, "configs/%s.cfg", config_name);
        strncpy(machine_config_name, config_name, 255);
        rpclog("arc_load_config_and_reset: machine_config_name=%s machine_config_file=%s\n", machine_config_name, machine_config_file);
        loadconfig();
        arc_init();
        user_sound_enabled = soundena;
}

void EMSCRIPTEN_KEEPALIVE arc_disc_change(int drive, char *fn)
{
        rpclog("arc_disc_change: drive=%i fn=%s\n", drive, fn);

        disc_close(drive);
        strcpy(discname[drive], fn);
        disc_load(drive, discname[drive]);
        ioc_discchange(drive);

}

void EMSCRIPTEN_KEEPALIVE arc_disc_eject(int drive)
{
        rpclog("arc_disc_eject: drive=%i\n", drive);
        ioc_discchange(drive);
        disc_close(drive);
        discname[drive][0] = 0;

}

/*Run flat out, without sound or video, until time_ms of emulated time.
  Calling it during a fast-forward moves the end point, and the progress
  still counts from where it started*/
void EMSCRIPTEN_KEEPALIVE arc_fast_forward(int time_ms)
{
        if (fast_forward_to_time_ms == 0)
                fast_forward_start_ms = total_emulation_millis;
        soundena = 0;
        skip_video_render = 1;
        fast_forward_to_time_ms = time_ms;
        rpclog("arc_fast_forward: %d\n", time_ms);
        fast_forward_js_progress(fast_forward_start_ms, total_emulation_millis, time_ms, 1);
}

void EMSCRIPTEN_KEEPALIVE arc_set_page_hidden(int hidden)
{
        page_hidden = hidden;
        if (fast_forward_to_time_ms == 0)
                skip_video_render = hidden;
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

}

void EMSCRIPTEN_KEEPALIVE arc_renderer_reset()
{
        
}

void EMSCRIPTEN_KEEPALIVE arc_set_display_mode(int new_display_mode)
{

        display_mode = new_display_mode;
        clearbitmap();
        setredrawall();

}

void EMSCRIPTEN_KEEPALIVE arc_set_dblscan(int new_dblscan)
{

        dblscan = new_dblscan;
        clearbitmap();

}


void EMSCRIPTEN_KEEPALIVE arc_set_sound_filter(int filter)
{

        if (filter >=0 && filter <= 2) {
                sound_filter = filter;
                sound_update_filter();
        }

}

void EMSCRIPTEN_KEEPALIVE arc_enable_sound(int enable) 
{
        user_sound_enabled = enable;
        /*Sound stays off until a fast-forward has finished*/
        if (fast_forward_to_time_ms == 0 && enable != soundena)
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
        arc_main_thread();
}

void arc_print_error(const char *format, ...) {

}