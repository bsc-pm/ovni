#include <assert.h>
#include "uthash.h"

#include "ovni.h"
#include "ovni_trace.h"
#include "emu.h"
#include "prv.h"
#include "chan.h"

/* --------------------------- init ------------------------------- */

void
hook_init_tampi(struct ovni_emu *emu)
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

		chan_th_init(th, CHAN_TAMPI_MODE, CHAN_TRACK_TH_RUNNING, row, prv_th, clock);
		chan_enable(&th->chan[CHAN_TAMPI_MODE], 1);
		chan_set(&th->chan[CHAN_TAMPI_MODE], ST_NULL);
		chan_enable(&th->chan[CHAN_TAMPI_MODE], 0);
	}

	/* Init the channels in all cpus */
	for(i=0; i<emu->total_ncpus; i++)
	{
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;

		chan_cpu_init(cpu, CHAN_TAMPI_MODE, CHAN_TRACK_TH_RUNNING, row, prv_cpu, clock);
	}
}

/* --------------------------- pre ------------------------------- */


static void
pre_send(struct ovni_emu *emu)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_SEND);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_SEND);
			break;
		default:
			  abort();
	}
}

static void
pre_recv(struct ovni_emu *emu)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_RECV);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_RECV);
			break;
		default:
			  abort();
	}
}

static void
pre_isend(struct ovni_emu *emu)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_ISEND);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_ISEND);
			break;
		default:
			  abort();
	}
}

static void
pre_irecv(struct ovni_emu *emu)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_IRECV);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_IRECV);
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
			chan_push(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_WAIT);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_WAIT);
			break;
		default:
			  abort();
	}
}

static void
pre_waitall(struct ovni_emu *emu)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_WAITALL);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_TAMPI_MODE], ST_TAMPI_WAITALL);
			break;
		default:
			  abort();
	}
}

void
hook_pre_tampi(struct ovni_emu *emu)
{
	assert(emu->cur_ev->header.model == 'T');

	switch(emu->cur_ev->header.category)
	{
		case 'S': pre_send(emu); break;
		case 'R': pre_recv(emu); break;
		case 's': pre_isend(emu); break;
		case 'r': pre_irecv(emu); break;
		case 'V': pre_wait(emu); break;
		case 'W': pre_waitall(emu); break;
		default:
			break;
	}
}
