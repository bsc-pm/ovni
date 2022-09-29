/* Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "chan.h"

#include "emu.h"
#include "prv.h"
#include "utlist.h"

static void
chan_init(struct ovni_chan *chan, enum chan_track track, int row, int type, FILE *prv, int64_t *clock)
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
	chan->lastst = -1;
}

static void
mark_dirty(struct ovni_chan *chan, enum chan_dirty dirty)
{
	if (chan->dirty != CHAN_CLEAN)
		die("mark_dirty: chan %d already dirty\n", chan->id);

	if (dirty == CHAN_CLEAN)
		die("mark_dirty: cannot use CHAN_CLEAN\n");

	dbg("adding dirty chan %d to list\n", chan->id);
	chan->dirty = dirty;
	DL_APPEND(*chan->update_list, chan);
}

void
chan_th_init(struct ovni_ethread *th,
	struct ovni_chan **update_list,
	enum chan id,
	enum chan_track track,
	int init_st,
	int enabled,
	int dirty,
	int row,
	FILE *prv,
	int64_t *clock)
{
	struct ovni_chan *chan;
	int prvth;

	chan = &th->chan[id];
	prvth = chan_to_prvtype[id];

	chan_init(chan, track, row, prvth, prv, clock);

	chan->id = id;
	chan->thread = th;
	chan->update_list = update_list;
	chan->enabled = enabled;
	chan->stack[chan->n++] = init_st;
	if (dirty)
		mark_dirty(chan, CHAN_DIRTY_ACTIVE);
}

void
chan_cpu_init(struct ovni_cpu *cpu,
	struct ovni_chan **update_list,
	enum chan id,
	enum chan_track track,
	int init_st,
	int enabled,
	int dirty,
	int row,
	FILE *prv,
	int64_t *clock)
{
	struct ovni_chan *chan;
	int prvcpu;

	chan = &cpu->chan[id];
	prvcpu = chan_to_prvtype[id];

	chan_init(chan, track, row, prvcpu, prv, clock);

	chan->id = id;
	chan->cpu = cpu;
	chan->update_list = update_list;
	chan->enabled = enabled;
	chan->stack[chan->n++] = init_st;
	if (dirty)
		mark_dirty(chan, CHAN_DIRTY_ACTIVE);
}

static void
chan_dump_update_list(struct ovni_chan *chan)
{
	struct ovni_chan *c;

	dbg("update list for chan %d at %p:\n",
		chan->id, (void *) chan);
	for (c = *chan->update_list; c; c = c->next) {
		dbg(" chan %d at %p\n", c->id, (void *) c);
	}
}

void
chan_enable(struct ovni_chan *chan, int enabled)
{
	/* Can be dirty */

	dbg("chan_enable chan=%d enabled=%d\n", chan->id, enabled);

	if (chan->enabled == enabled) {
		err("chan_enable: chan already in enabled=%d\n", enabled);
		abort();
	}

	chan->enabled = enabled;
	chan->t = *chan->clock;

	/* Only append if not dirty */
	if (!chan->dirty) {
		mark_dirty(chan, CHAN_DIRTY_ACTIVE);
	} else {
		dbg("already dirty chan %d: skip update list\n",
			chan->id);
		chan_dump_update_list(chan);
	}
}

void
chan_disable(struct ovni_chan *chan)
{
	chan_enable(chan, 0);
}

int
chan_is_enabled(struct ovni_chan *chan)
{
	return chan->enabled;
}

void
chan_set(struct ovni_chan *chan, int st)
{
	/* Can be dirty */

	dbg("chan_set chan %d st=%d\n", chan->id, st);

	if (!chan->enabled)
		die("chan_set: chan %d not enabled\n", chan->id);

	/* Only chan_set can set the 0 state */
	if (st < 0)
		die("chan_set: cannot set a negative state %d\n", st);

	/* Don't enforce this check if we are dirty because the channel was
	 * just enabled; it may collide with a new state 0 set via chan_set()
	 * used by the tracking channels */
	if (chan->dirty != CHAN_DIRTY_ACTIVE
		&& chan->lastst >= 0
		&& chan->lastst == st) {
		err("chan_set id=%d cannot emit the state %d twice\n",
			chan->id, st);
		abort();
	}

	if (chan->n == 0)
		chan->n = 1;

	chan->stack[chan->n - 1] = st;
	chan->t = *chan->clock;

	/* Only append if not dirty */
	if (!chan->dirty) {
		mark_dirty(chan, CHAN_DIRTY_VALUE);
	} else {
		dbg("already dirty chan %d: skip update list\n",
			chan->id);
		chan_dump_update_list(chan);
	}
}

void
chan_push(struct ovni_chan *chan, int st)
{
	dbg("chan_push chan %d st=%d\n", chan->id, st);

	if (!chan->enabled)
		die("chan_push: chan %d not enabled\n", chan->id);

	if (st <= 0) {
		err("chan_push: cannot push a non-positive state %d\n", st);
		abort();
	}

	/* Cannot be dirty */
	if (chan->dirty != CHAN_CLEAN)
		die("chan_push: chan %d not clean", chan->id);

	if (chan->lastst >= 0 && chan->lastst == st) {
		err("chan_push id=%d cannot emit the state %d twice\n",
			chan->id, st);
		abort();
	}

	if (chan->n >= MAX_CHAN_STACK) {
		err("chan_push: channel stack full\n");
		abort();
	}

	chan->stack[chan->n++] = st;
	chan->t = *chan->clock;

	mark_dirty(chan, CHAN_DIRTY_VALUE);
}

int
chan_pop(struct ovni_chan *chan, int expected_st)
{
	int st;

	dbg("chan_pop chan %d expected_st=%d\n", chan->id, expected_st);

	if (!chan->enabled)
		die("chan_pop: chan %d not enabled\n", chan->id);

	/* Cannot be dirty */
	if (chan->dirty != CHAN_CLEAN)
		die("chan_pop: chan %d not clean", chan->id);

	if (chan->n <= 0) {
		err("chan_pop: channel empty\n");
		abort();
	}

	st = chan->stack[chan->n - 1];

	if (expected_st >= 0 && st != expected_st) {
		err("chan_pop: unexpected channel state %d (expected %d)\n",
			st, expected_st);
		abort();
	}

	chan->n--;

	/* Take the current stack value */
	st = chan_get_st(chan);

	/* A st == 0 can be obtained when returning to the initial state */
	if (st < 0) {
		err("chan_pop: obtained negative state %d from the stack\n", st);
		abort();
	}

	if (chan->lastst >= 0 && chan->lastst == st) {
		err("chan_pop id=%d cannot emit the state %d twice\n",
			chan->id, st);
		abort();
	}

	chan->t = *chan->clock;

	mark_dirty(chan, CHAN_DIRTY_VALUE);

	return st;
}

void
chan_ev(struct ovni_chan *chan, int ev)
{
	dbg("chan_ev chan %d ev=%d\n", chan->id, ev);

	if (!chan->enabled)
		die("chan_ev: chan %d not enabled\n", chan->id);

	/* Cannot be dirty */
	if (chan->dirty != CHAN_CLEAN)
		die("chan_ev: chan %d is dirty\n", chan->id);

	if (ev <= 0)
		die("chan_ev: cannot emit non-positive state %d\n", ev);

	if (chan->lastst >= 0 && chan->lastst == ev)
		die("chan_ev id=%d cannot emit the state %d twice\n",
			chan->id, ev);

	chan->ev = ev;
	chan->t = *chan->clock;

	mark_dirty(chan, CHAN_DIRTY_VALUE);
}

int
chan_get_st(struct ovni_chan *chan)
{
	if (chan->enabled == 0)
		return chan->badst;

	if (chan->n == 0)
		return 0;

	if (chan->n < 0)
		die("chan_get_st: chan %d has negative n\n", chan->id);

	return chan->stack[chan->n - 1];
}

static void
emit(struct ovni_chan *chan, int64_t t, int state)
{
	if (chan->dirty == CHAN_CLEAN)
		die("emit: chan %d is not dirty\n", chan->id);

	/* A channel can only emit the same state as lastst if is dirty because
	 * it has been enabled or disabled. Otherwise is a bug (ie. you have two
	 * consecutive ovni events?) */
	if (chan->lastst != -1
		&& chan->dirty == CHAN_DIRTY_VALUE
		&& chan->lastst == state) {
		/* TODO: Print the raw clock of the offending event */
		die("emit: chan %d cannot emit the same state %d twice\n",
			chan->id, state);
	}

	if (chan->lastst != state) {
		prv_ev(chan->prv, chan->row, t, chan->type, state);

		chan->lastst = state;
	}
}

static void
emit_ev(struct ovni_chan *chan)
{
	int new, last;

	if (!chan->enabled)
		die("emit_ev: chan %d is not enabled\n", chan->id);

	if (chan->ev == -1)
		die("emit_ev: chan %d cannot emit -1 ev\n", chan->id);

	new = chan->ev;
	last = chan_get_st(chan);

	emit(chan, chan->t - 1, new);
	emit(chan, chan->t, last);

	chan->ev = -1;
}

static void
emit_st(struct ovni_chan *chan)
{
	int st;

	st = chan_get_st(chan);

	emit(chan, chan->t, st);
}

/* Emits either the current state or punctual event in the PRV file */
void
chan_emit(struct ovni_chan *chan)
{
	if (likely(chan->dirty == 0))
		return;

	dbg("chan_emit chan %d\n", chan->id);

	/* Emit badst if disabled */
	if (chan->enabled == 0) {
		/* No punctual events allowed when disabled */
		if (chan->ev != -1)
			die("chan_emit: no punctual event allowed when disabled\n");

		emit_st(chan);
		goto shower;
	}

	/* Otherwise, emit punctual event if any or the state */
	if (chan->ev != -1)
		emit_ev(chan);
	else
		emit_st(chan);

shower:
	chan->dirty = 0;
}
