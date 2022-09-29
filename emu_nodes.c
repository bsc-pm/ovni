/* Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "uthash.h"

#include "chan.h"
#include "emu.h"
#include "ovni.h"
#include "prv.h"

/* --------------------------- init ------------------------------- */

void
hook_init_nodes(struct ovni_emu *emu)
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
	for (i = 0; i < emu->total_nthreads; i++) {
		th = emu->global_thread[i];
		row = th->gindex + 1;
		uth = &emu->th_chan;

		chan_th_init(th, uth, CHAN_NODES_SUBSYSTEM, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
	}

	/* Init the channels in all cpus */
	for (i = 0; i < emu->total_ncpus; i++) {
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;
		ucpu = &emu->cpu_chan;

		chan_cpu_init(cpu, ucpu, CHAN_NODES_SUBSYSTEM, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
	}
}

/* --------------------------- pre ------------------------------- */

static void
pre_subsystem(struct ovni_emu *emu, int st)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan;

	th = emu->cur_thread;
	chan = &th->chan[CHAN_NODES_SUBSYSTEM];

	switch (emu->cur_ev->header.value) {
		case '[':
			chan_push(chan, st);
			break;
		case ']':
			chan_pop(chan, st);
			break;
		default:
			edie(emu, "unexpected value '%c' (expecting '[' or ']')\n",
				emu->cur_ev->header.value);
	}
}

void
hook_pre_nodes(struct ovni_emu *emu)
{
	if (emu->cur_ev->header.model != 'D')
		edie(emu, "hook_pre_nodes: unexpected event with model %c\n",
			emu->cur_ev->header.model);

	if (!emu->cur_thread->is_running)
		edie(emu, "hook_pre_nodes: current thread %d not running\n",
			emu->cur_thread->tid);

	switch (emu->cur_ev->header.category) {
		case 'R':
			pre_subsystem(emu, ST_NODES_REGISTER);
			break;
		case 'U':
			pre_subsystem(emu, ST_NODES_UNREGISTER);
			break;
		case 'W':
			pre_subsystem(emu, ST_NODES_IF0_WAIT);
			break;
		case 'I':
			pre_subsystem(emu, ST_NODES_IF0_INLINE);
			break;
		case 'T':
			pre_subsystem(emu, ST_NODES_TASKWAIT);
			break;
		case 'C':
			pre_subsystem(emu, ST_NODES_CREATE);
			break;
		case 'S':
			pre_subsystem(emu, ST_NODES_SUBMIT);
			break;
		case 'P':
			pre_subsystem(emu, ST_NODES_SPAWN);
			break;
		default:
			break;
	}
}
