#include <assert.h>
#include "uthash.h"

#include "ovni.h"
#include "ovni_trace.h"
#include "emu.h"
#include "prv.h"
#include "chan.h"

/* --------------------------- init ------------------------------- */

void
hook_init_openmp(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_cpu *cpu;
	struct ovni_trace *trace;
	int i, row, type;
	FILE *prv_th, *prv_cpu;
	int64_t *clock;

	clock = &emu->delta_time;
	prv_th = emu->prv_thread;
	prv_cpu = emu->prv_cpu;
	trace = &emu->trace;

	/* Init the channels in all threads */
	for(i=0; i<emu->total_nthreads; i++)
	{
		th = emu->global_thread[i];
		row = th->gindex + 1;

		chan_th_init(th, CHAN_OPENMP_MODE, CHAN_TRACK_TH_RUNNING, row, prv_th, clock);
		chan_enable(&th->chan[CHAN_OPENMP_MODE], 1);
		chan_set(&th->chan[CHAN_OPENMP_MODE], ST_NULL);
		chan_enable(&th->chan[CHAN_OPENMP_MODE], 0);
	}

	/* Init the channels in all cpus */
	for(i=0; i<emu->total_ncpus; i++)
	{
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;

		chan_cpu_init(cpu, CHAN_OPENMP_MODE, CHAN_TRACK_TH_RUNNING, row, prv_cpu, clock);
	}
}

/* --------------------------- pre ------------------------------- */


static void
pre_task(struct ovni_emu *emu)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_OPENMP_MODE], ST_OPENMP_TASK);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_OPENMP_MODE], ST_OPENMP_TASK);
			break;
		default:
			  abort();
	}
}

static void
pre_barrier(struct ovni_emu *emu)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_OPENMP_MODE], ST_OPENMP_PARALLEL);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_OPENMP_MODE], ST_OPENMP_PARALLEL);
			break;
		default:
			  abort();
	}
}

void
hook_pre_openmp(struct ovni_emu *emu)
{
	assert(emu->cur_ev->header.model == 'M');

	switch(emu->cur_ev->header.category)
	{
		case 'T': pre_task(emu); break;
		case 'P': pre_barrier(emu); break;
		default:
			break;
	}
}
