/* Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "uthash.h"

#include "chan.h"
#include "emu.h"
#include "ovni.h"
#include "prv.h"

/* --------------------------- init ------------------------------- */

void
hook_init_openmp(struct ovni_emu *emu)
{
	int64_t *clock = &emu->delta_time;
	FILE *prv_th = emu->prv_thread;
	FILE *prv_cpu = emu->prv_cpu;

	/* Init the channels in all threads */
	for (size_t i = 0; i < emu->total_nthreads; i++) {
		struct ovni_ethread *th = emu->global_thread[i];
		int row = th->gindex + 1;
		struct ovni_chan **uth = &emu->th_chan;

		chan_th_init(th, uth, CHAN_OPENMP_MODE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
	}

	/* Init the channels in all cpus */
	for (size_t i = 0; i < emu->total_ncpus; i++) {
		struct ovni_cpu *cpu = emu->global_cpu[i];
		int row = cpu->gindex + 1;
		struct ovni_chan **ucpu = &emu->cpu_chan;

		chan_cpu_init(cpu, ucpu, CHAN_OPENMP_MODE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
	}
}

/* --------------------------- pre ------------------------------- */

static void
pre_mode(struct ovni_emu *emu, int st)
{
	struct ovni_ethread *th = emu->cur_thread;
	struct ovni_chan *chan = &th->chan[CHAN_OPENMP_MODE];

	switch (emu->cur_ev->header.value) {
		case '[':
			chan_push(chan, st);
			break;
		case ']':
			chan_pop(chan, st);
			break;
		default:
			err("unexpected value '%c' (expecting '[' or ']')\n",
					emu->cur_ev->header.value);
			abort();
	}
}

void
hook_pre_openmp(struct ovni_emu *emu)
{
	if (emu->cur_ev->header.model != 'M')
		die("hook_pre_openmp: unexpected event with model %c\n",
				emu->cur_ev->header.model);

	if (!emu->cur_thread->is_active)
		die("hook_pre_openmp: current thread %d not active\n",
				emu->cur_thread->tid);

	switch (emu->cur_ev->header.category) {
		case 'T':
			pre_mode(emu, ST_OPENMP_TASK);
			break;
		case 'P':
			pre_mode(emu, ST_OPENMP_PARALLEL);
			break;
		default:
			break;
	}
}
