#include <assert.h>

#include "emu.h"
#include "prv.h"

void
chan_init(struct ovni_chan *chan, int row, int type, FILE *prv, int64_t *clock)
{
	chan->n = 0;
	chan->type = type;
	chan->enabled = 0;
	chan->badst = ST_NULL;
	chan->ev = -1;
	chan->prv = prv;
	chan->clock = clock;
	chan->t = -1;
	chan->row = row;
	chan->dirty = 0;
}

void
chan_enable(struct ovni_chan *chan, int enabled)
{
	chan->enabled = enabled;
}

int
chan_is_enabled(struct ovni_chan *chan)
{
	return chan->enabled;
}

void
chan_set(struct ovni_chan *chan, int st)
{
	dbg("chan_set st=%d", st);

	assert(chan->enabled);
	assert(chan->dirty == 0);

	if(chan->n == 0)
		chan->n = 1;

	chan->stack[chan->n] = st;
	chan->t = *chan->clock;
	chan->dirty = 1;
}

void
chan_push(struct ovni_chan *chan, int st)
{
	dbg("chan_push st=%d", st);

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

	dbg("chan_pop expexted_st=%d", expected_st);

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

	assert(chan->enabled);

	st = chan_get_st(chan);

	prv_ev(chan->prv, chan->row, chan->t, chan->type, st);
}

/* Emits either the current state or punctual event in the PRV file */
void
chan_emit(struct ovni_chan *chan)
{
	if(chan->dirty == 0)
		return;

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
