/*AudioWorklet audio backend. The emulator does all of the filtering and
  resampling to 48kHz itself (sound.c); this just copies each finished 50ms
  block into a ring in shared memory (see sound_ring.h) which the AudioWorklet
  plays. Gain and disc noise mixing are done by the worklet.*/
#include <math.h>
#include <string.h>
#include <emscripten.h>
#include "arc.h"
#include "plat_sound.h"
#include "disc.h"
#include "sound.h"
#include "sound_ring.h"

/*Implemented in js/audio.js*/
extern void browser_init_audio(int ctrl_ptr, int audio_ptr, int dd_ptr);
extern void browser_publish_audio(int ctrl_ptr, int index, int value);

#define DEFAULT_TARGET_FILL_FRAMES 4320 /*90ms, used until the worklet reports its own*/
#define PACE_MAX_ADJUST_MS 4

static sound_ring_t sound_ring;
static volatile int32_t *const ctrl = sound_ring.ctrl;

/*Our copies of the counters we own*/
static uint32_t audio_write_count, dd_write_count;
static int ring_initialised;

void sound_dev_init(void)
{
	/*This is called again whenever the machine is changed. The ring and the
	  counters stay as they are*/
	if (ring_initialised)
		return;
	ring_initialised = 1;

	browser_init_audio((int)(intptr_t)sound_ring.ctrl, (int)(intptr_t)sound_ring.audio,
	                   (int)(intptr_t)sound_ring.dd);
}

void sound_dev_close(void)
{
}

static int gain_to_q8(int db)
{
	return (int)(pow(10.0, (double)db / 20.0) * 256.0);
}

static int audio_running(void)
{
	return ring_initialised && ctrl[SOUND_CTRL_CTX_RUNNING];
}

/*Called with a finished 50ms block of 48kHz stereo audio, before gain*/
void sound_givebuffer(int16_t *buf)
{
	uint32_t read_count, first, frames_to_end;

	/*Nothing is listening, so there's no point building up a backlog that would
	  be stale by the time the AudioContext starts*/
	if (!audio_running())
		return;

	read_count = (uint32_t)ctrl[SOUND_CTRL_READ];
	if (SOUND_RING_FRAMES - (audio_write_count - read_count) < SOUND_BLOCK_FRAMES)
	{
		/*The pacing in sound_dev_pace_ms() should stop this ever happening*/
		ctrl[SOUND_CTRL_BLOCKS_DROPPED]++;
		return;
	}

	first = audio_write_count & (SOUND_RING_FRAMES - 1);
	frames_to_end = SOUND_RING_FRAMES - first;
	if (frames_to_end >= SOUND_BLOCK_FRAMES)
		memcpy(&sound_ring.audio[first * 2], buf, SOUND_BLOCK_FRAMES * 4);
	else
	{
		memcpy(&sound_ring.audio[first * 2], buf, frames_to_end * 4);
		memcpy(&sound_ring.audio[0], buf + frames_to_end * 2, (SOUND_BLOCK_FRAMES - frames_to_end) * 4);
	}

	ctrl[SOUND_CTRL_GAIN] = gain_to_q8(sound_gain);
	ctrl[SOUND_CTRL_DD_GAIN] = (disc_noise_gain >= DISC_NOISE_DISABLED) ? 0 : gain_to_q8(disc_noise_gain);
	ctrl[SOUND_CTRL_BLOCKS_WRITTEN]++;

	audio_write_count += SOUND_BLOCK_FRAMES;
	browser_publish_audio((int)(intptr_t)ctrl, SOUND_CTRL_WRITE, (int32_t)audio_write_count);
}

/*Called with 100ms of mono 44.1kHz disc noise*/
void sound_givebufferdd(int16_t *buf)
{
	uint32_t read_count, first, samples_to_end;

	if (!audio_running())
		return;

	read_count = (uint32_t)ctrl[SOUND_CTRL_DD_READ];
	if (SOUND_DD_FRAMES - (dd_write_count - read_count) < SOUND_DD_BLOCK)
		return;

	first = dd_write_count & (SOUND_DD_FRAMES - 1);
	samples_to_end = SOUND_DD_FRAMES - first;
	if (samples_to_end >= SOUND_DD_BLOCK)
		memcpy(&sound_ring.dd[first], buf, SOUND_DD_BLOCK * 2);
	else
	{
		memcpy(&sound_ring.dd[first], buf, samples_to_end * 2);
		memcpy(&sound_ring.dd[0], buf + samples_to_end, (SOUND_DD_BLOCK - samples_to_end) * 2);
	}

	dd_write_count += SOUND_DD_BLOCK;
	browser_publish_audio((int)(intptr_t)ctrl, SOUND_CTRL_DD_WRITE, (int32_t)dd_write_count);
}

void sound_dev_note_loop(double gap_ms, double run_wall_ms)
{
	int gap = (int)(gap_ms + 0.5), run = (int)(run_wall_ms + 0.5);

	if (!ring_initialised)
		return;
	if (gap > ctrl[SOUND_CTRL_LOOP_GAP_MAX])
		ctrl[SOUND_CTRL_LOOP_GAP_MAX] = gap;
	if (gap > SOUND_LOOP_LATE_MS)
		ctrl[SOUND_CTRL_LOOP_LATE]++;
	if (run > ctrl[SOUND_CTRL_RUN_MAX])
		ctrl[SOUND_CTRL_RUN_MAX] = run;
}

/*Use the audio device as the master clock while it's playing: run slightly
  more or less emulation than the wall clock says to keep the ring near its
  target fill. This changes how fast emulated time goes, not the pitch of the
  audio, as the worklet always plays at a fixed rate. Never run so far ahead
  that the ring would overflow.*/
int sound_dev_pace_ms(int wall_ms)
{
	int run_ms = wall_ms;
	int target_frames, free_frames, blocks_needed, progress_ms;
	double pending_ms, adjust_ms;

	if (!audio_running() || !soundena)
		return wall_ms;

	target_frames = ctrl[SOUND_CTRL_TARGET_FILL];
	if (target_frames <= 0)
		target_frames = DEFAULT_TARGET_FILL_FRAMES;

	/*Audio that's in the ring, plus the part of the current block that has been
	  emulated already but not yet delivered. Counting the latter removes the
	  50ms sawtooth from the measurement*/
	progress_ms = sound_get_block_progress_ms();
	pending_ms = (double)(audio_write_count - (uint32_t)ctrl[SOUND_CTRL_READ]) / 48.0 + progress_ms;

	adjust_ms = ((double)target_frames / 48.0 - pending_ms) / 8.0;
	if (adjust_ms > PACE_MAX_ADJUST_MS)
		adjust_ms = PACE_MAX_ADJUST_MS;
	if (adjust_ms < -PACE_MAX_ADJUST_MS)
		adjust_ms = -PACE_MAX_ADJUST_MS;
	run_ms += (int)adjust_ms;
	if (run_ms < 0)
		run_ms = 0;

	/*Each 50ms of emulation (and the one we may already be part way through)
	  can deliver another block*/
	free_frames = SOUND_RING_FRAMES - (int)(audio_write_count - (uint32_t)ctrl[SOUND_CTRL_READ]);
	for (;;)
	{
		blocks_needed = (progress_ms + run_ms) / 50 + 1;
		if (run_ms == 0 || blocks_needed * SOUND_BLOCK_FRAMES <= free_frames)
			break;
		run_ms--;
	}

	return run_ms;
}
