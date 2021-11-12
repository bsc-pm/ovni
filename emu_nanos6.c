#include <assert.h>
#include "uthash.h"

#include "ovni.h"
#include "ovni_trace.h"
#include "emu.h"
#include "prv.h"
#include "chan.h"

/* --------------------------- init ------------------------------- */

void
hook_init_nanos6(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_cpu *cpu;
	size_t i;
	int row;
	FILE *prv_th, *prv_cpu;
	int64_t *clock;
	struct ovni_chan **uth, **ucpu;

	clock = &emu->delta_time;
	prv_th = emu->prv_thread;
	prv_cpu = emu->prv_cpu;

	/* Init the channels in all threads */
	for(i=0; i<emu->total_nthreads; i++)
	{
		th = emu->global_thread[i];
		row = th->gindex + 1;
		uth = &emu->th_chan;

		chan_th_init(th, uth, CHAN_NANOS6_MODE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
	}

	/* Init the channels in all cpus */
	for(i=0; i<emu->total_ncpus; i++)
	{
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;
		ucpu = &emu->cpu_chan;

		chan_cpu_init(cpu, ucpu, CHAN_NANOS6_MODE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
	}
}

/* --------------------------- pre ------------------------------- */

static void
pre_register(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_REGISTER);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_REGISTER);
			break;
		default:
			abort();
	}
}

static void
pre_unregister(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_UNREGISTER);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_UNREGISTER);
			break;
		default:
			abort();
	}
}

static void
pre_wait(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_IF0_WAIT);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_IF0_WAIT);
			break;
		default:
			abort();
	}
}

static void
pre_inline(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_IF0_INLINE);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_IF0_INLINE);
			break;
		default:
			abort();
	}
}

static void
pre_taskwait(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_TASKWAIT);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_TASKWAIT);
			break;
		default:
			abort();
	}
}

static void
pre_create(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_CREATE);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_CREATE);
			break;
		default:
			abort();
	}
}

static void
pre_submit(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_SUBMIT);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_SUBMIT);
			break;
		default:
			abort();
	}
}

static void
pre_spawn(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_SPAWN);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_NANOS6_MODE], ST_NANOS6_SPAWN);
			break;
		default:
			abort();
	}
}

void
hook_pre_nanos6(struct ovni_emu *emu)
{
	assert(emu->cur_ev->header.model == 'L');

	switch(emu->cur_ev->header.category)
	{
		case 'R': pre_register(emu); break;
		case 'U': pre_unregister(emu); break;
		case 'W': pre_wait(emu); break;
		case 'I': pre_inline(emu); break;
		case 'T': pre_taskwait(emu); break;
		case 'C': pre_create(emu); break;
		case 'S': pre_submit(emu); break;
		case 'P': pre_spawn(emu); break;
		default:
			break;
	}
}
