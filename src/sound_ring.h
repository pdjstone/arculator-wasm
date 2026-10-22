/*Shared-memory audio transport, used to hand sound from the emulator (which
  runs in a Worker) to the AudioWorklet on the audio thread.

  The emulator does the VIDC sound DMA and volume/stereo mixing, but not the
  filtering or the resampling to 48kHz: the worklet does that (see
  frontend/audio-dsp.js). So what crosses is the raw stream, in base clock
  ticks, as two rings:

   records - one per VIDC sound DMA fetch: 16 stereo samples and the number of
             base clock ticks each one is held for (the zero order hold)
   events  - things that change how the records are interpreted, positioned
             exactly in the stream: clock changes, filter changes and audio
             dump start

  Starting (or restarting) the stream isn't an event. It goes in a reset slot
  of its own, so that it can never be held up by a full event ring: a restart
  is how the stream recovers when anything goes wrong, including a full ring.
  A restart throws away every record and event before it.

  The rings, the control block and the disc noise ring live in the static data
  of the wasm heap, which is a SharedArrayBuffer, so the worklet reads them in
  place with no copying. The emulator is the only writer of the records, events
  and the *_WRITE counters; the worklet is the only writer of the *_READ
  counters and the stats; the main thread sets CTX_RUNNING. All control words
  are Int32 so that JS can use Atomics on them. The counters are free running
  (modulo 2^32) - the ring index is the counter masked by the ring size.

  Counters are published from the emulator with Atomics.store via
  browser_publish_audio() (js/audio.js), which orders the data writes before
  the counter update.

  The layout is mirrored in frontend/audio-ring.js - keep the two in sync.*/
#ifndef SOUND_RING_H
#define SOUND_RING_H

#include <stdint.h>

#define SOUND_REC_COUNT   16384 /*power of two. 16 samples of at least 2 ticks each, so at least 349ms even with a 1.5MHz base clock*/
#define SOUND_EVENT_COUNT 256  /*power of two*/
#define SOUND_DD_BLOCKS   8     /*disc noise blocks, power of two (800ms)*/
#define SOUND_DD_BLOCK    4410  /*mono 44.1kHz samples per sound_givebufferdd() call (100ms)*/
#define SOUND_CTRL_WORDS  32
#define SOUND_LOOP_LATE_MS 40

/*Event codes*/
enum
{
	/*1 was RESET, which is now sent through the reset slot*/
	SOUND_EVENT_CLOCK = 2, /*New base clock: coefficients apply from record 'rec', value (new SAMP_INC) from output frame 'out_frame'*/
	SOUND_EVENT_FILTER,    /*New coefficients from record 'rec'*/
	SOUND_EVENT_DUMP       /*Capture 'value' output frames starting at output frame 'out_frame'*/
};

/*One VIDC sound DMA fetch*/
typedef struct sound_record_t
{
	int16_t period;      /*base clock ticks that each sample is held for*/
	int16_t reserved;
	int16_t samples[32]; /*16 pairs of left, right*/
	int16_t pad[2];
} sound_record_t;

typedef struct sound_event_t
{
	int32_t code;        /*SOUND_EVENT_*. Unused in the reset slot*/
	int32_t rec;         /*record index the event applies from (reset, CLOCK, FILTER)*/
	int32_t out_frame;   /*output frame index, counted from the last reset, that the event applies from (CLOCK, DUMP)*/
	int32_t value;       /*SAMP_INC (reset, CLOCK) or frame count (DUMP)*/
	int32_t tick0;       /*reset: value of the ticks written counter at record 'rec'*/
	int32_t evt_base;    /*reset: index of the first event of the new stream*/
	double coef[5];      /*IIR coefficients A0, A1, A2, B1, B2 (reset, CLOCK, FILTER)*/
} sound_event_t;

/*100ms of disc drive noise. ddnoise.c makes each block at the start of the
  100ms it covers, so it arrives ahead of the main audio for the same moment.
  The worklet plays it from the output frame whose base clock tick reaches
  'tick', which locks it to the main audio with no extra latency*/
typedef struct sound_dd_block_t
{
	int32_t tick;        /*ticks written counter (see TICKS_WRITE) at the moment the block starts*/
	int32_t seq;         /*RESET_SEQ of the stream it belongs to*/
	int16_t samples[SOUND_DD_BLOCK];
} sound_dd_block_t;

_Static_assert(sizeof(sound_record_t) == 72, "sound_record_t layout is mirrored in audio-ring.js");
_Static_assert(sizeof(sound_dd_block_t) == 8828, "sound_dd_block_t layout is mirrored in audio-ring.js");
_Static_assert(sizeof(sound_event_t) == 64, "sound_event_t layout is mirrored in audio-ring.js");

enum
{
	SOUND_CTRL_REC_WRITE = 0, /*emulator: records written*/
	SOUND_CTRL_REC_READ,      /*worklet: records consumed*/
	SOUND_CTRL_DD_WRITE,      /*emulator: disc noise blocks written*/
	SOUND_CTRL_DD_READ,       /*worklet: disc noise blocks finished with*/
	SOUND_CTRL_CTX_RUNNING,   /*main thread: AudioContext is running and the worklet is attached*/
	SOUND_CTRL_GAIN,          /*emulator: sound_gain as a Q8 multiplier*/
	SOUND_CTRL_DD_GAIN,       /*emulator: disc noise gain as a Q8 multiplier*/
	SOUND_CTRL_TARGET_FILL,   /*worklet: wanted fill, in output frames. It adapts to how often playback runs dry*/
	SOUND_CTRL_DUMP_STATE,    /*AUDIO_DUMP_* (sound.h) plus SOUND_DUMP_ERROR. Emulator sets WAITING and RECORDING, worklet FINISHED*/
	SOUND_CTRL_UNDERRUNS,     /*worklet: times playback ran dry*/
	SOUND_CTRL_SILENT_FRAMES, /*worklet: frames of silence output because of underruns*/
	SOUND_CTRL_TRIMS,         /*worklet: times it dropped audio because it had too much queued*/
	SOUND_CTRL_TRIM_FRAMES,   /*worklet: frames dropped by trims*/
	SOUND_CTRL_CLIPS,         /*worklet: output samples that clipped*/
	SOUND_CTRL_MIN_FILL,      /*worklet: minimum fill (output frames) since last reset by the main thread*/
	SOUND_CTRL_MAX_FILL,      /*worklet: maximum fill (output frames) since last reset by the main thread*/
	SOUND_CTRL_RECORDS_DROPPED, /*emulator: records dropped because the ring was full*/
	SOUND_CTRL_EVENTS_DROPPED,  /*emulator: events dropped because the ring was full*/
	SOUND_CTRL_DD_UNDERRUNS,  /*worklet: times the next disc noise block wasn't there when it was due*/
	SOUND_CTRL_LOOP_GAP_MAX,  /*emulator: longest gap (ms) between arcloop() calls since last reset by the main thread*/
	SOUND_CTRL_LOOP_LATE,     /*emulator: arcloop() calls that came more than SOUND_LOOP_LATE_MS after the previous one*/
	SOUND_CTRL_RUN_MAX,       /*emulator: longest wall time (ms) spent in arc_run() since last reset by the main thread*/
	SOUND_CTRL_EVT_WRITE,     /*emulator: events written*/
	SOUND_CTRL_EVT_READ,      /*worklet: events consumed*/
	SOUND_CTRL_TICKS_WRITE,   /*emulator: base clock ticks in the records written*/
	SOUND_CTRL_FRAMES_READ,   /*worklet: output frames played since the last RESET*/
	SOUND_CTRL_RESET_ACK,     /*worklet: the RESET_SEQ of the last reset it applied*/
	SOUND_CTRL_DUMP_FRAMES,   /*worklet: frames captured so far by the audio dump*/
	SOUND_CTRL_FRAME_WM,      /*emulator: output frame watermark - every event that applies to a frame before this has been published*/
	SOUND_CTRL_RESET_SEQ,     /*emulator: resets sent. The latest is in reset_slot[RESET_SEQ & 1]*/
	SOUND_CTRL_TRIM_FILL      /*worklet: fill (output frames) above which it throws audio away. Moves with the target*/
};

#define SOUND_DUMP_ERROR 4

typedef struct sound_ring_t
{
	int32_t ctrl[SOUND_CTRL_WORDS];
	/*Two, so that the emulator can fill one in while the worklet may still be
	  reading the other. The worklet checks RESET_SEQ is unchanged after reading
	  a slot, in case two resets happen in quick succession (a seqlock)*/
	sound_event_t reset_slot[2];
	sound_event_t events[SOUND_EVENT_COUNT];
	sound_record_t records[SOUND_REC_COUNT];
	sound_dd_block_t dd[SOUND_DD_BLOCKS];
} sound_ring_t;

/*Platform (js/audio.js) side of the stream, called by sound_worklet.c*/

/*The AudioContext is running and has a worklet attached, so there is someone to
  send the stream to*/
int sound_dev_live(void);
/*(Re)start the stream from the next record. This always succeeds*/
void sound_dev_reset(int samp_inc, int clock_hz, const double *a_coef, const double *b_coef);
/*Each of these publishes an event. They return 0 if the event ring is full.
  The event is lost, so for CLOCK and FILTER the stream has to be restarted,
  or the worklet would carry on with the wrong clock or filter*/
int sound_dev_clock(uint32_t out_frame, int samp_inc, int clock_hz, const double *a_coef, const double *b_coef);
int sound_dev_filter(const double *a_coef, const double *b_coef);
int sound_dev_dump(uint32_t out_frame, uint32_t frames);
/*Append one DMA fetch to the stream. samples holds 16 left, right pairs.
  Returns 0 if there was no room, in which case the stream has a hole in it
  and has to be restarted. Nothing is visible to the worklet until
  sound_dev_flush()*/
int sound_dev_record(int period, const int16_t *samples);
/*Publish the records sent since the last call. frame_wm is the output frame
  position that the emulation has reached (sound_stream_frame_position()). Any
  event sent from now on applies to that frame or later, so this is how the
  worklet knows that it has seen all of the events for the frames before it.
  Output frames are only counted from the first full 50ms block after a RESET,
  so they can lag behind the records.*/
void sound_dev_flush(uint32_t frame_wm);
int sound_dev_get_dump_state(void);
void sound_dev_set_dump_state(int state);

/*Adjust the amount of emulation to run in this arcloop() iteration so that the
  worklet has about its target fill queued, using the audio device as the master
  clock. wall_ms is the wall clock time since the previous iteration. Returns
  wall_ms unchanged when audio isn't playing*/
int sound_dev_pace_ms(int wall_ms);

/*Implemented in sound_worklet.c*/
/*The output frame that the emulation has reached, counted from the last reset*/
uint32_t sound_stream_frame_position(void);
/*There is a stream that the worklet has been told to follow*/
int sound_stream_active(void);
/*The base clock tick that emulated time has reached, on the same count as
  TICKS_WRITE. Records are written at the start of the ticks they cover, so
  this is TICKS_WRITE less what is left of the latest one*/
uint32_t sound_stream_tick_position(void);
/*Implemented in sound_emscripten.c: TICKS_WRITE*/
uint32_t sound_dev_ticks_written(void);
/*Publish what has been sent to the worklet. Call after each arc_run()*/
void sound_stream_flush(void);

/*Record how the emulator's main loop is behaving, for the glitch counters: the
  gap since the previous arcloop() call and the wall time the last arc_run()
  took, both in ms*/
void sound_dev_note_loop(double gap_ms, double run_wall_ms);

#endif
