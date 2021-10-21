#include <assert.h>

#include "emu.h"
#include "prv.h"

void
chan_init(struct ovni_chan *chan, int track, int row, int type, FILE *prv, int64_t *clock)
{
	chan->n = 0;
	chan->type = type;
	chan->enabled = 0;
	chan->badst = ST_NULL;
	chan->ev = -1;
	chan->prv = prv;
	chan->clock = clock;
	chan->t = *clock;
	chan->row = row;
	chan->dirty = 0;
	chan->track = track;
}

void
chan_th_init(struct ovni_ethread *th, int chan_index, int track, int row, FILE *prv, int64_t *clock)
{
	struct ovni_chan *chan;
	int prvth;

	chan = &th->chan[chan_index];
	prvth = chan_to_prvtype[chan_index][1];

	chan_init(chan, track, row, prvth, prv, clock);
}

void
chan_cpu_init(struct ovni_cpu *cpu, int chan_index, int track, int row, FILE *prv, int64_t *clock)
{
	struct ovni_chan *chan;
	int prvcpu;

	chan = &cpu->chan[chan_index];
	prvcpu = chan_to_prvtype[chan_index][2];

	chan_init(chan, track, row, prvcpu, prv, clock);
}

void
chan_enable(struct ovni_chan *chan, int enabled)
{
	/* Can be dirty */

	dbg("chan_enable chan=%p enabled=%d\n", (void*) chan, enabled);

	if(chan->enabled == enabled)
	{
		err("chan already in enabled=%d\n", enabled);
		abort();
	}

	chan->enabled = enabled;
	chan->t = *chan->clock;
	chan->dirty = 1;
}

int
chan_is_enabled(struct ovni_chan *chan)
{
	return chan->enabled;
}

void
chan_set(struct ovni_chan *chan, int st)
{
	//dbg("chan_set chan=%p st=%d\n", (void*) chan, st);

	assert(chan->enabled);
	//assert(chan->dirty == 0);

	if(chan->n == 0)
		chan->n = 1;

	chan->stack[chan->n - 1] = st;
	chan->t = *chan->clock;
	chan->dirty = 1;
}

void
chan_push(struct ovni_chan *chan, int st)
{
	//dbg("chan_push chan=%p st=%d\n", (void*) chan, st);

	assert(chan->enabled);
	assert(chan->dirty == 0);

	if(chan->n >= MAX_CHAN_STACK)
	{
		err("channel stack full\n");
		exit(EXIT_FAILURE);
	}

	chan->stack[chan->n++] = st;
	chan->t = *chan->clock;
	chan->dirty = 1;
}

int
chan_pop(struct ovni_chan *chan, int expected_st)
{
	int st;

	//dbg("chan_pop chan=%p expected_st=%d\n", (void*) chan, expected_st);

	assert(chan->enabled);
	assert(chan->dirty == 0);

	if(chan->n <= 0)
	{
		err("channel empty\n");
		exit(EXIT_FAILURE);
	}

	st = chan->stack[chan->n - 1];

	if(expected_st >= 0 && st != expected_st)
	{
		err("unexpected channel state %d (expected %d)\n",
				st, expected_st);
		exit(EXIT_FAILURE);
	}

	chan->n--;
	chan->t = *chan->clock;
	chan->dirty = 1;

	return st;
}

void
chan_ev(struct ovni_chan *chan, int ev)
{
	dbg("chan_ev chan=%p ev=%d\n", (void*) chan, ev);

	assert(chan->enabled);
	assert(chan->dirty == 0);
	assert(ev >= 0);

	chan->ev = ev;
	chan->t = *chan->clock;
	chan->dirty = 1;
}

int
chan_get_st(struct ovni_chan *chan)
{
	if(chan->enabled == 0)
		return chan->badst;

	if(chan->n == 0)
		return 0;

	assert(chan->n > 0);
	return chan->stack[chan->n-1];
}

static void
emit_ev(struct ovni_chan *chan)
{
	int new, last;

	assert(chan->enabled);
	assert(chan->ev != -1);

	new = chan->ev;
	last = chan_get_st(chan);

	prv_ev(chan->prv, chan->row, chan->t-1, chan->type, new);
	prv_ev(chan->prv, chan->row, chan->t, chan->type, last);

	chan->ev = -1;
}

static void
emit_st(struct ovni_chan *chan)
{
	int st;

	st = chan_get_st(chan);

	prv_ev(chan->prv, chan->row, chan->t, chan->type, st);
}

/* Emits either the current state or punctual event in the PRV file */
void
chan_emit(struct ovni_chan *chan)
{
	if(chan->dirty == 0)
		return;

	//dbg("chan_emit chan=%p\n", (void*) chan);

	/* Emit badst if disabled */
	if(chan->enabled == 0)
	{
		/* No punctual events allowed when disabled */
		assert(chan->ev == -1);

		emit_st(chan);
		goto shower;
	}

	/* Otherwise, emit punctual event if any or the state */
	if(chan->ev != -1)
		emit_ev(chan);
	else
		emit_st(chan);

shower:
	chan->dirty = 0;
}
