/* Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "uthash.h"

#include "chan.h"
#include "emu.h"
#include "ovni.h"
#include "prv.h"

/* --------------------------- init ------------------------------- */

void
hook_init_tampi(struct ovni_emu *emu)
{
	int64_t *clock = &emu->delta_time;
	FILE *prv_th = emu->prv_thread;
	FILE *prv_cpu = emu->prv_cpu;

	/* Init the channels in all threads */
	for (size_t i = 0; i < emu->total_nthreads; i++) {
		struct ovni_ethread *th = emu->global_thread[i];
		int row = th->gindex + 1;
		struct ovni_chan **uth = &emu->th_chan;

		chan_th_init(th, uth, CHAN_TAMPI_MODE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
	}

	/* Init the channels in all cpus */
	for (size_t i = 0; i < emu->total_ncpus; i++) {
		struct ovni_cpu *cpu = emu->global_cpu[i];
		int row = cpu->gindex + 1;
		struct ovni_chan **ucpu = &emu->cpu_chan;

		chan_cpu_init(cpu, ucpu, CHAN_TAMPI_MODE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
	}
}

/* --------------------------- pre ------------------------------- */

static void
pre_tampi_mode(struct ovni_emu *emu, int state)
{
	struct ovni_ethread *th = emu->cur_thread;

	switch (emu->cur_ev->header.value) {
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
	if (emu->cur_ev->header.model != 'T')
		edie(emu, "hook_pre_tampi: unexpected event with model %c\n",
			emu->cur_ev->header.model);

	switch (emu->cur_ev->header.category) {
		case 'S':
			pre_tampi_mode(emu, ST_TAMPI_SEND);
			break;
		case 'R':
			pre_tampi_mode(emu, ST_TAMPI_RECV);
			break;
		case 's':
			pre_tampi_mode(emu, ST_TAMPI_ISEND);
			break;
		case 'r':
			pre_tampi_mode(emu, ST_TAMPI_IRECV);
			break;
		case 'V':
			pre_tampi_mode(emu, ST_TAMPI_WAIT);
			break;
		case 'W':
			pre_tampi_mode(emu, ST_TAMPI_WAITALL);
			break;
		default:
			break;
	}
}
