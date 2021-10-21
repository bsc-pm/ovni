#include <assert.h>
#include "uthash.h"

#include "ovni.h"
#include "ovni_trace.h"
#include "emu.h"
#include "prv.h"
#include "chan.h"

/* This module manages the nos-v subsystem (ss) events, to track which
 * actions are being performed by each thread at a given time. A stack
 * of states is stored in each thread, so we remember the path of
 * execution. Events such as task received by a thread are emitted as
 * fake events with very short period. */

/* --------------------------- init ------------------------------- */

void
hook_init_nosv_ss(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	int i, row, type, track;
	FILE *prv;
	int64_t *clock;

	clock = &emu->delta_time;
	prv = emu->prv_thread;
	track = CHAN_TRACK_TH_RUNNING;

	/* Init the channels in all threads */
	for(i=0; i<emu->total_nthreads; i++)
	{
		th = emu->global_thread[i];
		row = th->gindex + 1;

		chan_th_init(th, CHAN_NOSV_SUBSYSTEM, track, row, prv, clock);
	}
}

/* --------------------------- pre ------------------------------- */

static void
pre_sched(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan;

	th = emu->cur_thread;
	chan = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case 'h':
			chan_push(chan, ST_SCHED_HUNGRY);
			break;
		case 'f': /* Fill: no longer hungry */
			chan_pop(chan, ST_SCHED_HUNGRY);
			break;
		case '[': /* Server enter */
			chan_push(chan, ST_SCHED_SERVING);
			break;
		case ']': /* Server exit */
			chan_pop(chan, ST_SCHED_SERVING);
			break;
		case '@': chan_ev(chan, EV_SCHED_SELF); break;
		case 'r': chan_ev(chan, EV_SCHED_RECV); break;
		case 's': chan_ev(chan, EV_SCHED_SEND); break;
		default:
			break;
	}
}

static void
pre_submit(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan;

	th = emu->cur_thread;
	chan = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case '[': chan_push(chan, ST_SCHED_SUBMITTING); break;
		case ']': chan_pop(chan, ST_SCHED_SUBMITTING); break;
		default:
			  break;
	}
}

static void
pre_memory(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan;

	th = emu->cur_thread;
	chan = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case '[': chan_push(chan, ST_MEM_ALLOCATING); break;
		case ']': chan_pop(chan, ST_MEM_ALLOCATING); break;
		default:
			  break;
	}
}

void
hook_pre_nosv_ss(struct ovni_emu *emu)
{
	assert(emu->cur_ev->header.model == 'V');

	switch(emu->cur_ev->header.class)
	{
		case 'S': pre_sched(emu); break;
		case 'U': pre_submit(emu); break;
		case 'M': pre_memory(emu); break;
		default:
			break;
	}
}

/* --------------------------- emit ------------------------------- */

void
hook_emit_nosv_ss(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan;

	th = emu->cur_thread;

	chan_emit(&th->chan[CHAN_NOSV_SUBSYSTEM]);
}

/* --------------------------- post ------------------------------- */

void
hook_post_nosv_ss(struct ovni_emu *emu)
{
}
