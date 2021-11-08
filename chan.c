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

static void
mark_dirty(struct ovni_chan *chan)
{
	assert(chan->dirty == 0);

	dbg("adding dirty chan %d to list\n", chan->id);
	chan->dirty = 1;
	DL_APPEND(*chan->update_list, chan);
}

void
chan_th_init(struct ovni_ethread *th,
		struct ovni_chan **update_list,
		enum chan id,
		int track,
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
		mark_dirty(chan);
}

void
chan_cpu_init(struct ovni_cpu *cpu,
		struct ovni_chan **update_list,
		enum chan id,
		int track,
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
		mark_dirty(chan);
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
		mark_dirty(chan);
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

	if(chan->n == 0)
		chan->n = 1;

	chan->stack[chan->n - 1] = st;
	chan->t = *chan->clock;

	/* Only append if not dirty */
	if(!chan->dirty)
	{
		mark_dirty(chan);
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

	/* Cannot be dirty */
	assert(chan->dirty == 0);

	if(chan->n >= MAX_CHAN_STACK)
	{
		err("channel stack full\n");
		exit(EXIT_FAILURE);
	}

	chan->stack[chan->n++] = st;
	chan->t = *chan->clock;

	mark_dirty(chan);
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
	chan->t = *chan->clock;

	mark_dirty(chan);

	return st;
}

void
chan_ev(struct ovni_chan *chan, int ev)
{
	dbg("chan_ev chan %d ev=%d\n", chan->id, ev);

	assert(chan->enabled);

	/* Cannot be dirty */
	assert(chan->dirty == 0);
	assert(ev >= 0);

	chan->ev = ev;
	chan->t = *chan->clock;

	mark_dirty(chan);
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
