/*JSCLI: lets the frontend run RISC OS * commands and get their output back.

  The work inside RISC OS is done by helper modules in the extension ROM
  (src/jscli).
  Each module serves one channel:
    0  JSCLI      runs commands from a transient callback, output from WrchV.
                  Also puts keys in the keyboard buffer
    1  JSCLIWimp  runs commands in a TaskWindow (desktop only). Also runs
                  sessions: TaskWindows that stay open and are sent input

  The modules call SWI ArcEm_JSCLI (&56AC5), with a reason code in r0:
    0 Register    r1 = channel, r2 = address of the module's pending word,
                  or 0 to deregister. Fails any command the channel was running.
                  Exit: r0 = JSCLI_ACK
    1 GetCommand  r1 = channel, r2 = buffer, r3 = buffer size,
                  r4 = bit mask of the kinds of request the module can take now
                  Exit: r0 = request id, or 0 if none is queued,
                        r1 = kind (JSCLI_KIND_*), r2 = target request id
                        (for input and close), r3 = length. The request is
                        copied to the buffer (a command NUL terminated)
    2 Output      r1 = request id, r2 = data, r3 = length
    3 Done        r1 = request id, r2 = error block, or 0 for success

  A request is running from GetCommand until Done. A session is a request
  that runs until its TaskWindow exits, and its output is reported with its
  id. Input and close requests name the session (or command) they're for as
  their target, and are done once the message has been sent.

  The pending word holds JSCLI_MAGIC in its top 24 bits, and the emulator
  sets its low byte to 1 while the channel has commands queued. So JSCLI can
  check for work on TickerV with a load rather than a SWI. If the magic has
  gone, the module has gone without deregistering (e.g. its workspace was
  freed), so the channel is dropped rather than writing to the address.

  JSCLIWimp instead calls GetCommand on every null event while it's idle, so
  the time of a channel's last call shows whether it's still polling: a Wimp
  task stops when the desktop is suspended (F12, a single-tasking program).

  On the JS side, arc_cli_queue() queues a command and arc_cli_queue_data()
  the other kinds of request, and the worker gets the results through
  self.jscliOutput() and self.jscliDone().*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif
#include "arc.h"
#include "jscli.h"
#include "timer.h"

#define JSCLI_CHANNELS   2
#define JSCLI_QUEUE_LEN  16
#define JSCLI_MAX_CMD    255
#define JSCLI_MAX_DATA   256    /*the modules' command buffer*/
#define JSCLI_MAX_INPUT  232    /*the data in a Wimp message*/
#define JSCLI_MAX_OUTPUT 4096
#define JSCLI_MAX_RUNNING 8     /*JSCLIWimp's TaskWindow slots*/

#define JSCLI_MAGIC      0x434c4900
#define JSCLI_ACK        0x4a53434c

enum {
	JSCLI_REGISTER = 0,
	JSCLI_GET_COMMAND = 1,
	JSCLI_OUTPUT = 2,
	JSCLI_DONE = 3
};

enum {
	JSCLI_KIND_CLI = 0,
	JSCLI_KIND_KEYS = 1,
	JSCLI_KIND_OPEN = 2,    /*start a session, running the command (or a * prompt)*/
	JSCLI_KIND_INPUT = 3,   /*send bytes to a session's TaskWindow*/
	JSCLI_KIND_CLOSE = 4    /*kill a session's (or command's) TaskWindow*/
};

typedef struct {
	int id;
	int kind;
	int target;             /*for input and close*/
	int len;
	uint8_t *data;
} jscli_request_t;

typedef struct {
	int id;
	int kind;
} jscli_running_t;

typedef struct {
	uint32_t pending_addr;  /*0 if no module is registered*/
	int max_running;
	int n_running;          /*taken by GetCommand and not yet Done*/
	jscli_running_t running[JSCLI_MAX_RUNNING];
	long long last_call_ms; /*emulated time of the module's last call, or -1*/
	int queue_len;
	jscli_request_t queue[JSCLI_QUEUE_LEN];
} jscli_channel_t;

/*Emulated time in ms. Not total_emulation_millis, which only moves on when
  arc_run() returns, in steps of up to 500 ms*/
static long long jscli_now_ms(void)
{
	return tsc / TIMER_USEC / 1000;
}

static jscli_channel_t channels[JSCLI_CHANNELS] = {
	{ .max_running = 1, .last_call_ms = -1 },
	{ .max_running = JSCLI_MAX_RUNNING, .last_call_ms = -1 }
};
static int next_id = 1;

#ifdef __EMSCRIPTEN__
EM_JS(void, jscli_js_output, (int id, const uint8_t *data, int len), {
	if (self.jscliOutput) self.jscliOutput(id, HEAPU8.slice(data, data + len));
});
EM_JS(void, jscli_js_done, (int id, int failed, uint32_t errnum, const uint8_t *msg, int msglen), {
	if (self.jscliDone) self.jscliDone(id, failed, errnum >>> 0, HEAPU8.slice(msg, msg + msglen));
});
#else
static void jscli_js_output(int id, const uint8_t *data, int len)
{
	printf("jscli %d output: %.*s\n", id, len, (const char *)data);
}
static void jscli_js_done(int id, int failed, uint32_t errnum, const uint8_t *msg, int msglen)
{
	printf("jscli %d done: %s %u %.*s\n", id, failed ? "error" : "ok", errnum, msglen, (const char *)msg);
}
#endif

static void jscli_fail(int id, const char *msg)
{
	jscli_js_done(id, 1, 0, (const uint8_t *)msg, strlen(msg));
}

static void jscli_remove_running(jscli_channel_t *ch, int i)
{
	ch->n_running--;
	memmove(ch->running + i, ch->running + i + 1, (ch->n_running - i) * sizeof(jscli_running_t));
}

/*Fail the running requests and, if `flush_queue`, the queued ones*/
static void jscli_fail_channel(jscli_channel_t *ch, int flush_queue, const char *msg)
{
	for (int i = 0; i < ch->n_running; i++)
		jscli_fail(ch->running[i].id, msg);
	ch->n_running = 0;
	if (flush_queue) {
		for (int i = 0; i < ch->queue_len; i++) {
			jscli_fail(ch->queue[i].id, msg);
			free(ch->queue[i].data);
		}
		ch->queue_len = 0;
	}
}

/*The module's pending word, or NULL (dropping the channel) if it's gone*/
static uint32_t *jscli_pending_word(jscli_channel_t *ch)
{
	uint32_t a = ch->pending_addr;
	uint32_t *p = NULL;

	if (!a)
		return NULL;
	if (!(a & 3) && a < 0x2000000 && modepritablew[MEMMODE_SUPER][memstat[(a >> 12) & 0x3fff]])
		p = (uint32_t *)&mempoint[(a >> 12) & 0x3fff][a];
	if (!p || (*p & 0xffffff00) != JSCLI_MAGIC) {
		rpclog("JSCLI: pending word at %08x has gone, dropping channel %d\n", a, (int)(ch - channels));
		ch->pending_addr = 0;
		jscli_fail_channel(ch, 1, "The JSCLI module has gone");
		return NULL;
	}
	return p;
}

static void jscli_update_pending(jscli_channel_t *ch)
{
	uint32_t *p = jscli_pending_word(ch);

	if (p)
		*p = JSCLI_MAGIC | (ch->queue_len ? 1 : 0);
}

static jscli_channel_t *jscli_channel(uint32_t n)
{
	return n < JSCLI_CHANNELS ? &channels[n] : NULL;
}

/*The channel running request `id`, with its index in `*index`*/
static jscli_channel_t *jscli_channel_running(int id, int *index)
{
	for (int i = 0; i < JSCLI_CHANNELS; i++)
		for (int j = 0; j < channels[i].n_running; j++)
			if (channels[i].running[j].id == id) {
				if (index)
					*index = j;
				return &channels[i];
			}
	return NULL;
}

static int jscli_queued(jscli_channel_t *ch, int id)
{
	for (int i = 0; i < ch->queue_len; i++)
		if (ch->queue[i].id == id)
			return 1;
	return 0;
}

/*The first queued request the module can take: one of the kinds in `mask`,
  and not input or close for a session that hasn't been taken yet. Returns its
  index, or -1*/
static int jscli_next_request(jscli_channel_t *ch, uint32_t mask)
{
	for (int i = 0; i < ch->queue_len; i++) {
		jscli_request_t *req = &ch->queue[i];
		if (!(mask & (1u << req->kind)))
			continue;
		if (req->target && jscli_queued(ch, req->target))
			continue;
		return i;
	}
	return -1;
}

void jscli_swi(uint32_t *regs)
{
	jscli_channel_t *ch;

	switch (regs[0]) {
	case JSCLI_REGISTER:
		ch = jscli_channel(regs[1]);
		if (!ch)
			return;
		/*A (re)registering module has lost whatever it was running*/
		jscli_fail_channel(ch, !regs[2], regs[2] ? "The JSCLI module was restarted" : "The JSCLI module has gone");
		ch->pending_addr = regs[2];
		ch->last_call_ms = regs[2] ? jscli_now_ms() : -1;
		jscli_update_pending(ch);
		regs[0] = JSCLI_ACK;
		return;

	case JSCLI_GET_COMMAND: {
		uint32_t buf = regs[2], size = regs[3];
		ch = jscli_channel(regs[1]);
		regs[0] = 0;
		if (!ch)
			return;
		ch->last_call_ms = jscli_now_ms();
		int next = jscli_next_request(ch, regs[4]);
		if (next < 0 || !size)
			return;
		jscli_request_t req = ch->queue[next];
		ch->queue_len--;
		memmove(ch->queue + next, ch->queue + next + 1, (ch->queue_len - next) * sizeof(jscli_request_t));
		int terminated = req.kind == JSCLI_KIND_CLI || req.kind == JSCLI_KIND_OPEN;
		uint32_t len = req.len;
		if (terminated && len > size - 1)
			len = size - 1;
		else if (len > size)
			len = size;
		for (uint32_t i = 0; i < len; i++)
			writememb(buf + i, req.data[i]);
		if (terminated)
			writememb(buf + len, 0);
		free(req.data);
		if (ch->n_running == ch->max_running) {
			jscli_fail(ch->running[0].id, "The JSCLI module started another command");
			jscli_remove_running(ch, 0);
		}
		ch->running[ch->n_running++] = (jscli_running_t){ req.id, req.kind };
		jscli_update_pending(ch);
		regs[0] = req.id;
		regs[1] = req.kind;
		regs[2] = req.target;
		regs[3] = len;
		return;
	}

	case JSCLI_OUTPUT: {
		static uint8_t data[JSCLI_MAX_OUTPUT];
		uint32_t ptr = regs[2], len = regs[3];
		ch = jscli_channel_running(regs[1], NULL);
		if (!ch)
			return;
		ch->last_call_ms = jscli_now_ms();
		while (len) {
			uint32_t n = len < JSCLI_MAX_OUTPUT ? len : JSCLI_MAX_OUTPUT;
			for (uint32_t i = 0; i < n; i++)
				data[i] = readmemb(ptr + i);
			jscli_js_output(regs[1], data, n);
			ptr += n;
			len -= n;
		}
		return;
	}

	case JSCLI_DONE: {
		uint32_t err = regs[2];
		int index;
		ch = jscli_channel_running(regs[1], &index);
		if (!ch)
			return;
		ch->last_call_ms = jscli_now_ms();
		jscli_remove_running(ch, index);
		if (err) {
			uint8_t msg[256];
			int len = 0;
			while (len < 255 && (msg[len] = readmemb(err + 4 + len)) != 0)
				len++;
			jscli_js_done(regs[1], 1, readmeml(err), msg, len);
		} else {
			jscli_js_done(regs[1], 0, 0, (const uint8_t *)"", 0);
		}
		return;
	}
	}
}

/*Called when the ARM is reset: the modules are gone*/
void jscli_reset(void)
{
	for (int i = 0; i < JSCLI_CHANNELS; i++) {
		channels[i].pending_addr = 0;
		channels[i].last_call_ms = -1;
		jscli_fail_channel(&channels[i], 1, "The emulator was reset");
	}
}

static int jscli_queue(int channel, int kind, int target, const uint8_t *data, int len)
{
	jscli_channel_t *ch = jscli_channel(channel);
	int max_len;

	if (!ch || !jscli_pending_word(ch))
		return -1;
	if (ch->queue_len == JSCLI_QUEUE_LEN)
		return -2;
	switch (kind) {
	case JSCLI_KIND_CLI:
	case JSCLI_KIND_OPEN:
		max_len = JSCLI_MAX_CMD;
		break;
	case JSCLI_KIND_INPUT:
		max_len = JSCLI_MAX_INPUT;
		break;
	case JSCLI_KIND_CLOSE:
		max_len = 0;
		break;
	default:
		max_len = JSCLI_MAX_DATA;
	}
	if (len > max_len)
		return -3;
	if (kind == JSCLI_KIND_KEYS ? channel != 0 :
	    kind != JSCLI_KIND_CLI && channel != 1)
		return -4;
	/*Input can go to a session that hasn't started yet, but there's nothing to
	  close until it has (a queued one can be cancelled)*/
	if (kind == JSCLI_KIND_INPUT || kind == JSCLI_KIND_CLOSE) {
		if (jscli_channel_running(target, NULL) != ch && (kind == JSCLI_KIND_CLOSE || !jscli_queued(ch, target)))
			return -5;
	} else {
		target = 0;
	}
	jscli_request_t *req = &ch->queue[ch->queue_len++];
	req->id = next_id++;
	req->kind = kind;
	req->target = target;
	req->len = len;
	req->data = malloc(len ? len : 1);
	memcpy(req->data, data, len);
	jscli_update_pending(ch);
	return req->id;
}

/*Queue a command for `channel`. Returns its id (> 0), which the results are
  reported with, or -1 if no module serves the channel, -2 if its queue is
  full, -3 if the command is too long*/
int EMSCRIPTEN_KEEPALIVE arc_cli_queue(int channel, const char *cmd)
{
	return jscli_queue(channel, JSCLI_KIND_CLI, 0, (const uint8_t *)cmd, strlen(cmd));
}

/*Queue a request of another kind for `channel`:
    keys (JSCLI): at most JSCLI_MAX_DATA bytes
    open a session (JSCLIWimp): the command to run in it, or nothing for a
      * prompt
    input for session `target` (JSCLIWimp): at most JSCLI_MAX_INPUT bytes
    close (JSCLIWimp): kill the TaskWindow of the session or command `target`
  Returns as arc_cli_queue(), or -4 if the channel's module can't do that,
  -5 if `target` isn't running (or, for input, queued)*/
int EMSCRIPTEN_KEEPALIVE arc_cli_queue_data(int channel, int kind, int target, const uint8_t *data, int len)
{
	if (kind < JSCLI_KIND_KEYS || kind > JSCLI_KIND_CLOSE)
		return -4;
	return jscli_queue(channel, kind, target, data, len);
}

/*Bit n is set if a module serves channel n*/
int EMSCRIPTEN_KEEPALIVE arc_cli_status(void)
{
	int status = 0;

	for (int i = 0; i < JSCLI_CHANNELS; i++)
		if (jscli_pending_word(&channels[i]))
			status |= 1 << i;
	return status;
}

/*Remove a queued request, so it won't be run or reported. Returns 1 if it
  was removed, 0 if it's running (JSCLIWimp's can be killed with a close
  request), -1 if it's neither (it has finished)*/
int EMSCRIPTEN_KEEPALIVE arc_cli_cancel(int id)
{
	for (int i = 0; i < JSCLI_CHANNELS; i++) {
		jscli_channel_t *ch = &channels[i];
		if (jscli_channel_running(id, NULL) == ch)
			return 0;
		for (int j = 0; j < ch->queue_len; j++) {
			if (ch->queue[j].id == id) {
				free(ch->queue[j].data);
				ch->queue_len--;
				memmove(ch->queue + j, ch->queue + j + 1, (ch->queue_len - j) * sizeof(jscli_request_t));
				jscli_update_pending(ch);
				return 1;
			}
		}
	}
	return -1;
}

/*Emulated ms since `channel`'s module last called the emulator, or -1 if no
  module serves it*/
int EMSCRIPTEN_KEEPALIVE arc_cli_idle_ms(int channel)
{
	jscli_channel_t *ch = jscli_channel(channel);

	if (!ch || !jscli_pending_word(ch) || ch->last_call_ms < 0)
		return -1;
	return jscli_now_ms() - ch->last_call_ms;
}

/*Whether `channel` is running a command (or VDU bytes or keys), not counting
  sessions*/
int EMSCRIPTEN_KEEPALIVE arc_cli_busy(int channel)
{
	jscli_channel_t *ch = jscli_channel(channel);

	if (ch)
		for (int i = 0; i < ch->n_running; i++)
			if (ch->running[i].kind <= JSCLI_KIND_KEYS)
				return 1;
	return 0;
}
