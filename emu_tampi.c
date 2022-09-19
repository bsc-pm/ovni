/* Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "uthash.h"

#include "ovni.h"
#include "emu.h"
#include "prv.h"
#include "chan.h"

/* --------------------------- init ------------------------------- */

void
hook_init_tampi(struct ovni_emu *emu)
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

		chan_th_init(th, uth, CHAN_TAMPI_MODE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
	}

	/* Init the channels in all cpus */
	for(i=0; i<emu->total_ncpus; i++)
	{
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;
		ucpu = &emu->cpu_chan;

		chan_cpu_init(cpu, ucpu, CHAN_TAMPI_MODE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
	}
}

/* --------------------------- pre ------------------------------- */

static void
pre_tampi_mode(struct ovni_emu *emu, int state)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(&th->chan[CHAN_TAMPI_MODE], state);
			break;
		case ']':
			chan_pop(&th->chan[CHAN_TAMPI_MODE], state);
			break;
		default:
			edie(emu, "unexpected event value %c for tampi mode\n",
					emu->cur_ev->header.value);
	}
}

void
hook_pre_tampi(struct ovni_emu *emu)
{
	if(emu->cur_ev->header.model != 'T')
		edie(emu, "hook_pre_tampi: unexpected event with model %c\n",
				emu->cur_ev->header.model);

	switch(emu->cur_ev->header.category)
	{
		case 'S': pre_tampi_mode(emu, ST_TAMPI_SEND); break;
		case 'R': pre_tampi_mode(emu, ST_TAMPI_RECV); break;
		case 's': pre_tampi_mode(emu, ST_TAMPI_ISEND); break;
		case 'r': pre_tampi_mode(emu, ST_TAMPI_IRECV); break;
		case 'V': pre_tampi_mode(emu, ST_TAMPI_WAIT); break;
		case 'W': pre_tampi_mode(emu, ST_TAMPI_WAITALL); break;
		default:
			break;
	}
}
