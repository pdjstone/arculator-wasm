/*Shared-memory audio ring, used to hand finished audio from the emulator (which
  runs in a Worker) to the AudioWorklet on the audio thread.

  The ring lives in the static data of the wasm heap, which is a
  SharedArrayBuffer, so the worklet reads it in place with no copying. The
  emulator is the only writer of the audio data and of the WRITE/DD_WRITE
  counters; the worklet is the only writer of READ/DD_READ and the stats; the
  main thread sets CTX_RUNNING and FLUSH. All control words are Int32 so that
  JS can use Atomics on them. WRITE and READ are free-running frame counters
  (modulo 2^32) - the ring index is the counter masked by the ring size.

  Counters are published from the emulator with Atomics.store via
  browser_publish_audio() (js/audio.js), which orders the sample writes before
  the counter update.

  The layout is mirrored in frontend/audio-ring.js - keep the two in sync.*/
#ifndef SOUND_RING_H
#define SOUND_RING_H

#include <stdint.h>

#define SOUND_RING_FRAMES 16384 /*stereo frames at 48kHz, power of two (341ms)*/
#define SOUND_DD_FRAMES   16384 /*mono samples at 44.1kHz, power of two (371ms)*/
#define SOUND_BLOCK_FRAMES 2400 /*stereo frames per sound_givebuffer() call*/
#define SOUND_DD_BLOCK     4410 /*mono samples per sound_givebufferdd() call*/
#define SOUND_CTRL_WORDS   32
#define SOUND_LOOP_LATE_MS 40

enum
{
	SOUND_CTRL_WRITE = 0,   /*emulator: stereo frames written*/
	SOUND_CTRL_READ,        /*worklet: stereo frames consumed*/
	SOUND_CTRL_DD_WRITE,    /*emulator: ddnoise samples written*/
	SOUND_CTRL_DD_READ,     /*worklet: ddnoise samples consumed*/
	SOUND_CTRL_CTX_RUNNING, /*main thread: AudioContext is running and the worklet is attached*/
	SOUND_CTRL_GAIN,        /*emulator: sound_gain as a Q8 multiplier*/
	SOUND_CTRL_DD_GAIN,     /*emulator: disc noise gain as a Q8 multiplier*/
	SOUND_CTRL_TARGET_FILL, /*worklet: wanted fill, in stereo frames*/
	SOUND_CTRL_FLUSH,       /*main thread: bumped on resume so the worklet drops stale audio*/
	SOUND_CTRL_UNDERRUNS,   /*worklet: times playback ran dry*/
	SOUND_CTRL_SILENT_FRAMES, /*worklet: frames of silence output because of underruns*/
	SOUND_CTRL_TRIMS,       /*worklet: times it dropped audio because the ring was overfull*/
	SOUND_CTRL_TRIM_FRAMES, /*worklet: frames dropped by trims*/
	SOUND_CTRL_CLIPS,       /*worklet: output samples that clipped*/
	SOUND_CTRL_MIN_FILL,    /*worklet: minimum fill (frames) since last reset*/
	SOUND_CTRL_MAX_FILL,    /*worklet: maximum fill (frames) since last reset*/
	SOUND_CTRL_BLOCKS_DROPPED, /*emulator: 50ms blocks dropped because the ring was full*/
	SOUND_CTRL_BLOCKS_WRITTEN, /*emulator: 50ms blocks written*/
	SOUND_CTRL_DD_UNDERRUNS, /*worklet: times the ddnoise stream ran dry*/
	SOUND_CTRL_LOOP_GAP_MAX, /*emulator: longest gap (ms) between arcloop() calls since last reset by the main thread*/
	SOUND_CTRL_LOOP_LATE,   /*emulator: arcloop() calls that came more than SOUND_LOOP_LATE_MS after the previous one*/
	SOUND_CTRL_RUN_MAX      /*emulator: longest wall time (ms) spent in arc_run() since last reset by the main thread*/
};

typedef struct sound_ring_t
{
	int32_t ctrl[SOUND_CTRL_WORDS];
	int16_t audio[SOUND_RING_FRAMES * 2];
	int16_t dd[SOUND_DD_FRAMES];
} sound_ring_t;

/*Adjust the amount of emulation to run in this arcloop() iteration so that the
  audio ring stays near its target fill, using the audio device as the master
  clock. wall_ms is the wall clock time since the previous iteration. Returns
  wall_ms unchanged when audio isn't playing*/
int sound_dev_pace_ms(int wall_ms);

/*Record how the emulator's main loop is behaving, for the glitch counters: the
  gap since the previous arcloop() call and the wall time the last arc_run()
  took, both in ms*/
void sound_dev_note_loop(double gap_ms, double run_wall_ms);

#endif
