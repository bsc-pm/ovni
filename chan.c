/*
 * Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "chan.h"

#include <assert.h>

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
	assert(chan->dirty == CHAN_CLEAN);
	assert(dirty != CHAN_CLEAN);

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
	prvth = CHAN_PRV_TH(id);

	chan_init(chan, track, row, prvth, prv, clock);

	chan->id = id;
	chan->thread = th;
	chan->update_list = update_list;
	chan->enabled = enabled;
	chan->stack[chan->n++] = init_st;
	if(dirty)
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
	prvcpu = CHAN_PRV_CPU(id);

	chan_init(chan, track, row, prvcpu, prv, clock);

	chan->id = id;
	chan->cpu = cpu;
	chan->update_list = update_list;
	chan->enabled = enabled;
	chan->stack[chan->n++] = init_st;
	if(dirty)
		mark_dirty(chan, CHAN_DIRTY_ACTIVE);
}

static void
chan_dump_update_list(struct ovni_chan *chan)
{
	struct ovni_chan *c;

	dbg("update list for chan %d at %p:\n",
			chan->id, (void *) chan);
	for(c = *chan->update_list; c; c = c->next)
	{
		dbg(" chan %d at %p\n", c->id, (void *) c);
	}
}

void
chan_enable(struct ovni_chan *chan, int enabled)
{
	/* Can be dirty */

	dbg("chan_enable chan=%d enabled=%d\n", chan->id, enabled);

	if(chan->enabled == enabled)
	{
		err("chan already in enabled=%d\n", enabled);
		abort();
	}

	chan->enabled = enabled;
	chan->t = *chan->clock;

	/* Only append if not dirty */
	if(!chan->dirty)
	{
		mark_dirty(chan, CHAN_DIRTY_ACTIVE);
	}
	else
	{
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

	assert(chan->enabled);

	/* Only chan_set can set the 0 state */
	if(st < 0)
	{
		err("chan_set: cannot set a negative state %d\n", st);
		abort();
	}

	/* Don't enforce this check if we are dirty because the channel was
	 * just enabled; it may collide with a new state 0 set via chan_set()
	 * used by the tracking channels */
	if(chan->dirty != CHAN_DIRTY_ACTIVE
			&& chan->lastst >= 0
			&& chan->lastst == st)
	{
		err("chan_set id=%d cannot emit the state %d twice\n",
				chan->id, st);
		abort();
	}

	if(chan->n == 0)
		chan->n = 1;

	chan->stack[chan->n - 1] = st;
	chan->t = *chan->clock;

	/* Only append if not dirty */
	if(!chan->dirty)
	{
		mark_dirty(chan, CHAN_DIRTY_VALUE);
	}
	else
	{
		dbg("already dirty chan %d: skip update list\n",
				chan->id);
		chan_dump_update_list(chan);
	}
}

void
chan_enable_and_set(struct ovni_chan *chan, int st)
{
	chan_enable(chan, 1);
	chan_set(chan, st);
}

void
chan_push(struct ovni_chan *chan, int st)
{
	dbg("chan_push chan %d st=%d\n", chan->id, st);

	assert(chan->enabled);

	if(st <= 0)
	{
		err("chan_push: cannot push a non-positive state %d\n", st);
		abort();
	}

	/* Cannot be dirty */
	assert(chan->dirty == 0);

	if(chan->lastst >= 0 && chan->lastst == st)
	{
		err("chan_push id=%d cannot emit the state %d twice\n",
				chan->id, st);
		abort();
	}

	if(chan->n >= MAX_CHAN_STACK)
	{
		err("channel stack full\n");
		exit(EXIT_FAILURE);
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

	assert(chan->enabled);

	/* Cannot be dirty */
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

	/* Take the current stack value */
	st = chan_get_st(chan);

	/* A st == 0 can be obtained when returning to the initial state */
	if(st < 0)
	{
		err("chan_pop: obtained negative state %d from the stack\n", st);
		abort();
	}

	if(chan->lastst >= 0 && chan->lastst == st)
	{
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

	assert(chan->enabled);

	/* Cannot be dirty */
	assert(chan->dirty == 0);

	if(ev <= 0)
	{
		err("chan_ev: cannot emit non-positive state %d\n", ev);
		abort();
	}

	if(chan->lastst >= 0 && chan->lastst == ev)
	{
		err("chan_ev id=%d cannot emit the state %d twice\n",
				chan->id, ev);
		abort();
	}

	chan->ev = ev;
	chan->t = *chan->clock;

	mark_dirty(chan, CHAN_DIRTY_VALUE);
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
emit(struct ovni_chan *chan, int64_t t, int state)
{
	assert(chan->dirty != CHAN_CLEAN);

	/* A channel can only emit the same state as lastst if is dirty because
	 * it has been enabled or disabled. Otherwise is a bug (ie. you have two
	 * consecutive ovni events?) */
	if(chan->lastst != -1
			&& chan->dirty == CHAN_DIRTY_VALUE
			&& chan->lastst == state)
	{
		/* TODO: Print the raw clock of the offending event */
		err("chan: id=%d cannot emit the same state %d twice\n",
				chan->id, state);
		abort();
	}

	if(chan->lastst != state)
	{
		prv_ev(chan->prv, chan->row, t, chan->type, state);

		chan->lastst = state;
	}
}

static void
emit_ev(struct ovni_chan *chan)
{
	int new, last;

	assert(chan->enabled);
	assert(chan->ev != -1);

	new = chan->ev;
	last = chan_get_st(chan);

	emit(chan, chan->t-1, new);
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
	if(likely(chan->dirty == 0))
		return;

	dbg("chan_emit chan %d\n", chan->id);

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
