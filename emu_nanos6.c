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

		chan_th_init(th, uth, CHAN_NANOS6_SUBSYSTEM, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
	}

	/* Init the channels in all cpus */
	for(i=0; i<emu->total_ncpus; i++)
	{
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;
		ucpu = &emu->cpu_chan;

		chan_cpu_init(cpu, ucpu, CHAN_NANOS6_SUBSYSTEM, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
	}
}

/* --------------------------- pre ------------------------------- */

static void
pre_subsystem(struct ovni_emu *emu, int st)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan;

	th = emu->cur_thread;
	chan = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(chan, st);
			break;
		case ']':
			chan_pop(chan, st);
			break;
		default:
			err("unexpected value '%c' (expecting '[' or ']')\n", emu->cur_ev->header.value);
			abort();
	}
}

void
hook_pre_nanos6(struct ovni_emu *emu)
{
	// Ensure that the thread is running
	assert(emu->cur_thread->is_running != 0);
	assert(emu->cur_ev->header.model == 'L');

	switch(emu->cur_ev->header.category)
	{
		case 'R': pre_subsystem(emu, ST_NANOS6_REGISTER); break;
		case 'U': pre_subsystem(emu, ST_NANOS6_UNREGISTER); break;
		case 'W': pre_subsystem(emu, ST_NANOS6_IF0_WAIT); break;
		case 'I': pre_subsystem(emu, ST_NANOS6_IF0_INLINE); break;
		case 'T': pre_subsystem(emu, ST_NANOS6_TASKWAIT); break;
		case 'C': pre_subsystem(emu, ST_NANOS6_CREATE); break;
		case 'S': pre_subsystem(emu, ST_NANOS6_SUBMIT); break;
		case 'P': pre_subsystem(emu, ST_NANOS6_SPAWN); break;
		default:
			break;
	}
}
