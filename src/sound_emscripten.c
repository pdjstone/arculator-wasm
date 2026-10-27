/*AudioWorklet audio backend. sound_worklet.c turns the VIDC's sound DMA into a stream
  of records and events; this puts them in rings in shared memory (see
  sound_ring.h) for the AudioWorklet, which filters and resamples them to 48kHz
  (frontend/audio-dsp.js), applies the gain and mixes in the disc noise.*/
#include <math.h>
#include <string.h>
#include <emscripten.h>
#include "arc.h"
#include "plat_sound.h"
#include "disc.h"
#include "sound.h"
#include "sound_ring.h"

/*Implemented in js/audio.js*/
extern void browser_init_audio(int ctrl_ptr, int reset_ptr, int event_ptr, int record_ptr, int dd_ptr);

#define DEFAULT_TARGET_FILL_FRAMES 4800 /*100ms, used until the worklet reports its own*/
#define PACE_MAX_ADJUST_MS 4
#define PACE_STALL_MS 250 /*If the worklet makes no progress for this long, stop pacing against it*/
#define PACE_TRIM_MARGIN_MS 20 /*How far below the worklet's trim level to stop when catching up*/

static sound_ring_t sound_ring;
static volatile int32_t *const ctrl = sound_ring.ctrl;
#define CTRL_PTR ((int)(intptr_t)sound_ring.ctrl)

/*Publish a counter in the control block. This is the same instruction as the
  worklet's Atomics.store/load (it needs -matomics), so the ring data written
  before it is visible to the worklet by the time the new value is*/
static void publish(int index, int32_t value)
{
	__atomic_store_n(&ctrl[index], value, __ATOMIC_SEQ_CST);
}

/*Our copies of the counters we own*/
static uint32_t rec_write_count, event_write_count, dd_write_count; /*dd_write_count is in blocks*/
static uint32_t ticks_written;
static uint32_t resets_sent;
/*The first record, event and disc noise block of the stream that the worklet
  is following, or will be once it sees our latest reset. Those before them
  are never read*/
static uint32_t rec_read_floor, event_read_floor, dd_read_floor;
static int ring_initialised;

/*What the stream currently looks like, for pacing*/
static int cur_clock_hz = 1000000;
static int cur_period = 2;

void sound_dev_init(void)
{
	/*This is called again whenever the machine is changed. The rings and the
	  counters stay as they are*/
	if (ring_initialised)
		return;
	ring_initialised = 1;

	browser_init_audio(CTRL_PTR, (int)(intptr_t)sound_ring.reset_slot, (int)(intptr_t)sound_ring.events,
	                   (int)(intptr_t)sound_ring.records, (int)(intptr_t)sound_ring.dd);
}

void sound_dev_close(void)
{
}

static int gain_to_q8(int db)
{
	return (int)(pow(10.0, (double)db / 20.0) * 256.0);
}

int sound_dev_live(void)
{
	return ring_initialised && ctrl[SOUND_CTRL_CTX_RUNNING];
}

/*Ring entries the worklet hasn't finished with. It moves to the start of the
  new stream when it sees a reset, so until it publishes that, its read
  position from the old stream would make the ring look fuller than it is*/
static uint32_t in_use(int read_index, uint32_t floor, uint32_t write_count)
{
	uint32_t read = (uint32_t)ctrl[read_index];

	if ((int32_t)(read - floor) < 0)
		read = floor;
	return write_count - read;
}

static sound_event_t *event_alloc(int code)
{
	sound_event_t *e;

	if (in_use(SOUND_CTRL_EVT_READ, event_read_floor, event_write_count) >= SOUND_EVENT_COUNT)
	{
		ctrl[SOUND_CTRL_EVENTS_DROPPED]++;
		return NULL;
	}

	e = &sound_ring.events[event_write_count & (SOUND_EVENT_COUNT - 1)];
	memset(e, 0, sizeof(*e));
	e->code = code;
	/*Events apply from the next record to be written*/
	e->rec = (int32_t)rec_write_count;
	return e;
}

static void event_set_coefs(sound_event_t *e, const double *a_coef, const double *b_coef)
{
	e->coef[0] = a_coef[0];
	e->coef[1] = a_coef[1];
	e->coef[2] = a_coef[2];
	e->coef[3] = b_coef[1];
	e->coef[4] = b_coef[2];
}

static void event_publish(void)
{
	event_write_count++;
	publish(SOUND_CTRL_EVT_WRITE, (int32_t)event_write_count);
}

void sound_dev_reset(int samp_inc, int clock_hz, const double *a_coef, const double *b_coef)
{
	/*The slot the worklet isn't looking at*/
	sound_event_t *e = &sound_ring.reset_slot[(resets_sent + 1) & 1];

	memset(e, 0, sizeof(*e));
	e->rec = (int32_t)rec_write_count;
	e->value = samp_inc;
	e->tick0 = (int32_t)ticks_written;
	e->evt_base = (int32_t)event_write_count;
	event_set_coefs(e, a_coef, b_coef);

	/*Everything before this is thrown away*/
	rec_read_floor = rec_write_count;
	event_read_floor = event_write_count;
	dd_read_floor = dd_write_count;
	cur_clock_hz = clock_hz;
	resets_sent++;

	/*Output frames are counted from the reset, so the watermark starts again
	  from zero. The worklet reads the watermark before RESET_SEQ, so by the
	  time it can see the reset it can see this*/
	publish(SOUND_CTRL_FRAME_WM, 0);
	publish(SOUND_CTRL_RESET_SEQ, (int32_t)resets_sent);
}

int sound_dev_clock(uint32_t out_frame, int samp_inc, int clock_hz, const double *a_coef, const double *b_coef)
{
	sound_event_t *e = event_alloc(SOUND_EVENT_CLOCK);

	cur_clock_hz = clock_hz;
	if (!e)
		return 0;
	e->out_frame = (int32_t)out_frame;
	e->value = samp_inc;
	event_set_coefs(e, a_coef, b_coef);
	event_publish();
	return 1;
}

int sound_dev_filter(const double *a_coef, const double *b_coef)
{
	sound_event_t *e = event_alloc(SOUND_EVENT_FILTER);

	if (!e)
		return 0;
	event_set_coefs(e, a_coef, b_coef);
	event_publish();
	return 1;
}

int sound_dev_dump(uint32_t out_frame, uint32_t frames)
{
	sound_event_t *e = event_alloc(SOUND_EVENT_DUMP);

	if (!e)
		return 0;
	e->out_frame = (int32_t)out_frame;
	e->value = (int32_t)frames;
	event_publish();
	return 1;
}

int sound_dev_get_dump_state(void)
{
	return ctrl[SOUND_CTRL_DUMP_STATE];
}

void sound_dev_set_dump_state(int state)
{
	ctrl[SOUND_CTRL_DUMP_STATE] = state;
}

static uint32_t records_in_use(void)
{
	return in_use(SOUND_CTRL_REC_READ, rec_read_floor, rec_write_count);
}

/*Append one VIDC sound DMA fetch to the record ring. Called by sound_worklet.c for
  every fetch while the stream is valid. samples holds the 16 left, right
  pairs after volume and stereo mixing, and period is the number of base clock
  ticks each of them is held for, which the worklet needs to rebuild the zero
  order hold.

  The record isn't visible to the worklet yet: this only updates our own
  counters, and sound_dev_flush() publishes them. That saves calling into JS
  for every fetch.

  Returns 0, and drops the record, if the ring is full. The stream then has a
  hole in it, so the caller has to restart it with a RESET rather than carry
  on: the worklet would otherwise splice audio from either side of the gap
  together, and the frame positions of later events would be wrong.*/
int sound_dev_record(int period, const int16_t *samples)
{
	sound_record_t *rec;

	if (records_in_use() >= SOUND_REC_COUNT)
	{
		/*The pacing in sound_dev_pace_ms() should stop this ever happening*/
		ctrl[SOUND_CTRL_RECORDS_DROPPED]++;
		return 0;
	}

	rec = &sound_ring.records[rec_write_count & (SOUND_REC_COUNT - 1)];
	rec->period = period;
	memcpy(rec->samples, samples, sizeof(rec->samples));

	rec_write_count++;
	/*The worklet works out where it is in the stream from the ticks, so this
	  has to count exactly what the records hold*/
	ticks_written += 16 * period;
	/*sound_dev_pace_ms() uses this to work out how much emulated time the
	  free space in the ring will hold*/
	cur_period = period;
	return 1;
}

uint32_t sound_dev_ticks_written(void)
{
	return ticks_written;
}

void sound_dev_flush(uint32_t frame_wm)
{
	static int last_gain_db, last_dd_gain_db;
	static int gains_valid;

	/*The worklet applies the gains, so they're only published when they change*/
	if (!gains_valid || sound_gain != last_gain_db)
	{
		last_gain_db = sound_gain;
		ctrl[SOUND_CTRL_GAIN] = gain_to_q8(sound_gain);
	}
	if (!gains_valid || disc_noise_gain != last_dd_gain_db)
	{
		last_dd_gain_db = disc_noise_gain;
		ctrl[SOUND_CTRL_DD_GAIN] = (disc_noise_gain >= DISC_NOISE_DISABLED) ? 0 : gain_to_q8(disc_noise_gain);
	}
	gains_valid = 1;

	/*The worklet reads these in the opposite order to this*/
	publish(SOUND_CTRL_REC_WRITE, (int32_t)rec_write_count);
	publish(SOUND_CTRL_TICKS_WRITE, (int32_t)ticks_written);
	publish(SOUND_CTRL_FRAME_WM, (int32_t)frame_wm);
}

/*Called with 100ms of mono 44.1kHz disc noise, at the start of the 100ms it
  covers. It's tagged with the tick it starts at, so the worklet can play it in
  step with the main audio. Only sent while there's a stream to tie it to*/
void sound_givebufferdd(int16_t *buf)
{
	sound_dd_block_t *block;

	if (!sound_dev_live() || !sound_stream_active())
		return;

	if (in_use(SOUND_CTRL_DD_READ, dd_read_floor, dd_write_count) >= SOUND_DD_BLOCKS)
		return; /*The worklet is far behind, or not playing it at all*/

	block = &sound_ring.dd[dd_write_count & (SOUND_DD_BLOCKS - 1)];
	block->tick = (int32_t)sound_stream_tick_position();
	block->seq = (int32_t)resets_sent;
	memcpy(block->samples, buf, sizeof(block->samples));

	dd_write_count++;
	publish(SOUND_CTRL_DD_WRITE, (int32_t)dd_write_count);
}

/*Called by arcloop() after each arc_run(), to keep counters of how well the
  emulator is keeping up. None of this affects the audio itself: it's there so
  that when the worklet reports underruns, window.arcAudioStats() can show
  whether the emulator was the cause.

   gap_ms      - wall time since the previous arcloop() call. Long gaps mean
                 the browser didn't run the loop when it should have (a
                 background tab, a busy main thread, a slow previous frame)
   run_wall_ms - wall time this arcloop() call has taken, which is mostly
                 arc_run(). If that is long the emulated machine is too busy
                 to run in real time

  LOOP_GAP_MAX and RUN_MAX are maximums, which the main thread resets with
  arcAudioStats(true). As it can reset them between our read and our write,
  a reset can occasionally be lost, which is harmless for a statistic.
  LOOP_LATE counts gaps longer than SOUND_LOOP_LATE_MS, which is long enough
  to empty a good part of the worklet's queue.*/
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
  more or less emulation than the wall clock says to keep the worklet's queue
  near its target fill. This changes how fast emulated time goes, not the pitch
  of the audio, as the worklet always plays at a fixed rate. Never run so far
  ahead that the record ring would overflow.*/
int sound_dev_pace_ms(int wall_ms)
{
	int run_ms = wall_ms, target_frames, trim_frames, max_ms;
	int32_t pending_frames;
	uint32_t free_records;
	double pending_ms, adjust_ms;

	static int32_t last_rec_read, last_frames_read;
	static double last_progress_ms;
	double now_ms;

	/*Only pace against the worklet when there's a stream for it to follow.
	  Otherwise there is nothing to measure: sound_stream_frame_position()
	  counts on from the last reset (or from boot) whether or not anything is
	  sent, which would look like a huge backlog and stop the emulator. And as
	  the stream only (re)starts when the emulator runs, that would be for good*/
	if (!sound_dev_live() || !sound_stream_active())
		return wall_ms;

	/*If the worklet stops making progress, for whatever reason, pacing against
	  it would stop the emulator, and nothing would ever start it again. Losing
	  the audio is much better than that. Once it's been stuck for a while,
	  just run at the wall clock speed: without the ring space check below, the
	  ring then overflows, which restarts the stream*/
	now_ms = emscripten_get_now();
	if (ctrl[SOUND_CTRL_REC_READ] != last_rec_read || ctrl[SOUND_CTRL_FRAMES_READ] != last_frames_read)
	{
		last_rec_read = ctrl[SOUND_CTRL_REC_READ];
		last_frames_read = ctrl[SOUND_CTRL_FRAMES_READ];
		last_progress_ms = now_ms;
	}
	else if (records_in_use() <= 1)
		last_progress_ms = now_ms; /*Nothing to consume (it may still be part way through the last record)*/
	if (now_ms - last_progress_ms > PACE_STALL_MS)
	{
		static double last_log_ms;

		if (now_ms - last_log_ms > 5000)
		{
			rpclog("sound_dev_pace_ms: the audio worklet is stuck, so the emulator is ignoring it\n");
			last_log_ms = now_ms;
		}
		return wall_ms;
	}

	/*Until the worklet has applied our latest reset, FRAMES_READ is meaningless,
	  so just run at the wall clock speed*/
	if ((uint32_t)ctrl[SOUND_CTRL_RESET_ACK] == resets_sent)
	{
		target_frames = ctrl[SOUND_CTRL_TARGET_FILL];
		if (target_frames <= 0)
			target_frames = DEFAULT_TARGET_FILL_FRAMES;

		/*What the worklet could play now: the output frames that the emulation
		  has reached and it hasn't played yet*/
		pending_frames = (int32_t)(sound_stream_frame_position() - (uint32_t)ctrl[SOUND_CTRL_FRAMES_READ]);
		if (pending_frames < 0)
			pending_frames = 0;
		pending_ms = (double)pending_frames / 48.0;

		adjust_ms = ((double)target_frames / 48.0 - pending_ms) / 8.0;
		if (adjust_ms > PACE_MAX_ADJUST_MS)
			adjust_ms = PACE_MAX_ADJUST_MS;
		if (adjust_ms < -PACE_MAX_ADJUST_MS)
			adjust_ms = -PACE_MAX_ADJUST_MS;
		run_ms += (int)adjust_ms;

		/*After a stall (a slow arc_run(), or the tab in the background) the wall
		  clock says to run a lot at once, which would overfill the worklet and
		  make it throw audio away. The audio is the master clock, so drop the
		  lost time instead. Stopping a little short of where the worklet starts
		  trimming still leaves room above the target, which is a cushion
		  against the next stall*/
		trim_frames = ctrl[SOUND_CTRL_TRIM_FILL];
		if (trim_frames <= target_frames)
			trim_frames = target_frames + target_frames / 2;
		max_ms = (int)((double)trim_frames / 48.0 - PACE_TRIM_MARGIN_MS - pending_ms);
		if (run_ms > max_ms)
			run_ms = max_ms;
		if (run_ms < 0)
			run_ms = 0;
	}

	/*Whatever happens, don't write more than there's room for. This matters
	  most just after a fast-forward, when wall_ms can be several hundred ms.
	  Each fetch is 16 samples of 'period' ticks*/
	free_records = SOUND_REC_COUNT - records_in_use();
	if (free_records > SOUND_REC_COUNT)
		free_records = 0;
	max_ms = (int)(((double)free_records - 4.0) * 16.0 * cur_period * 1000.0 / (double)cur_clock_hz);
	if (run_ms > max_ms)
		run_ms = max_ms < 0 ? 0 : max_ms;

	return run_ms;
}
