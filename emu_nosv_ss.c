#include <assert.h>
#include "uthash.h"

#include "ovni.h"
#include "ovni_trace.h"
#include "emu.h"
#include "prv.h"

/* This module manages the nos-v subsystem (ss) events, to track which
 * actions are being performed by each thread at a given time. A stack
 * of states is stored in each thread, so we remember the path of
 * execution. Events such as task received by a thread are emitted as
 * fake events with very short period. */


/* --------------------------- ss helpers ------------------------------- */

static void
ss_init(struct ovni_ethread *t)
{
	t->nss = 0;
}

static void
ss_push(struct ovni_ethread *t, int st)
{
	if(t->nss >= MAX_SS_STACK)
	{
		err("thread %d subsystem stack full\n", t->tid);
		exit(EXIT_FAILURE);
	}

	t->ss[t->nss] = st;
	t->nss++;
}

static int
ss_pop(struct ovni_ethread *t)
{
	int st;

	if(t->nss <= 0)
	{
		err("thread %d subsystem stack empty\n", t->tid);
		exit(EXIT_FAILURE);
	}

	st = t->ss[t->nss - 1];
	t->nss--;

	return st;
}

static void
ss_ev(struct ovni_ethread *th, int ev)
{
	th->ss_event = ev;
}

/* --------------------------- pre ------------------------------- */

static void
pre_sched(struct ovni_emu *emu)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;
	switch(emu->cur_ev->header.value)
	{
		case 'h':
			ss_push(th, ST_SCHED_HUNGRY);
			break;
		case 'f': /* Fill: no longer hungry */
			ss_pop(th);
			break;
		case '[': /* Server enter */
			ss_push(th, ST_SCHED_SERVING);
			break;
		case ']': /* Server exit */
			ss_pop(th);
			break;
		case '@': ss_ev(th, EV_SCHED_SELF); break;
		case 'r': ss_ev(th, EV_SCHED_RECV); break;
		case 's': ss_ev(th, EV_SCHED_SEND); break;
		default:
			break;
	}
}

static void
pre_submit(struct ovni_emu *emu)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;
	switch(emu->cur_ev->header.value)
	{
		case '[': ss_push(th, ST_SCHED_SUBMITTING); break;
		case ']': ss_pop(th); break;
		default:
			  break;
	}
}

void
hook_pre_nosv_ss(struct ovni_emu *emu)
{
	switch(emu->cur_ev->header.class)
	{
		case 'S': pre_sched(emu); break;
		case 'U': pre_submit(emu); break;
		default:
			  break;
	}
}

/* --------------------------- emit ------------------------------- */

static void
emit_thread_state(struct ovni_emu *emu, struct ovni_ethread *th,
		int type, int st)
{
	int row;
	
	row = th->gindex + 1;

	prv_ev_thread(emu, row, type, st);
}

void
hook_emit_nosv_ss(struct ovni_emu *emu)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;

//	/* Only emit a state if needed */
//	if(th->ss_event != EV_NULL)
//	{
//		emit_thread_state(emu, th, PTT_SUBSYSTEM,
//				th->ss_event);
//	}
//	else

	if(th->nss > 0)
	{
		emit_thread_state(emu, th, PTT_SUBSYSTEM,
				th->ss[th->nss - 1]);
	}
	else
	{
		emit_thread_state(emu, th, PTT_SUBSYSTEM, ST_NULL);
	}
}

/* --------------------------- post ------------------------------- */

void
hook_post_nosv_ss(struct ovni_emu *emu)
{
	emu->cur_thread->ss_event = EV_NULL;
}
