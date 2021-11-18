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

#include "ovni.h"
#include "emu.h"
#include "prv.h"
#include "chan.h"
#include "utlist.h"

#include <assert.h>

/* The emulator ovni module provides the execution model by tracking the thread
 * state and which threads run in each CPU */

/* --------------------------- init ------------------------------- */

void
hook_init_ovni(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_cpu *cpu;
	struct ovni_chan **uth, **ucpu;
	size_t i;
	int row;
	FILE *prv_th, *prv_cpu;
	int64_t *clock;

	clock = &emu->delta_time;
	prv_th = emu->prv_thread;
	prv_cpu = emu->prv_cpu;

	/* Init the ovni channels in all threads */
	for(i=0; i<emu->total_nthreads; i++)
	{
		th = emu->global_thread[i];
		row = th->gindex + 1;
		uth = &emu->th_chan;

		chan_th_init(th, uth, CHAN_OVNI_TID,   CHAN_TRACK_TH_RUNNING, th->tid,       0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_OVNI_PID,   CHAN_TRACK_TH_RUNNING, th->proc->pid, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_OVNI_CPU,   CHAN_TRACK_NONE,       -1,            0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_OVNI_STATE, CHAN_TRACK_NONE,       TH_ST_UNKNOWN, 1, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_OVNI_FLUSH, CHAN_TRACK_NONE,       ST_NULL,       1, 1, row, prv_th, clock);
	}

	/* Init the ovni channels in all cpus */
	for(i=0; i<emu->total_ncpus; i++)
	{
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;
		ucpu = &emu->cpu_chan;

		chan_cpu_init(cpu, ucpu, CHAN_OVNI_TID,       CHAN_TRACK_TH_RUNNING,   0, 1, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_OVNI_PID,       CHAN_TRACK_TH_RUNNING,   0, 1, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_OVNI_NRTHREADS, CHAN_TRACK_NONE,         0, 1, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_OVNI_FLUSH,     CHAN_TRACK_TH_RUNNING,   0, 0, 1, row, prv_cpu, clock);
	}
}

/* --------------------------- pre ------------------------------- */

/* Update the tracking channel if needed */
static void
chan_tracking_update(struct ovni_chan *chan, struct ovni_ethread *th)
{
	int enabled;

	assert(th);

	switch (chan->track)
	{
		case CHAN_TRACK_TH_RUNNING:
			enabled = th->is_running;
			break;
		case CHAN_TRACK_TH_ACTIVE:
			enabled = th->is_active;
			break;
		default:
			dbg("ignoring thread %d chan %d with track=%d\n",
				th->tid, chan->id, chan->track);
			return;
	}

	/* The channel is already in the proper state */
	if(chan_is_enabled(chan) == enabled)
		return;

	dbg("thread %d changes state to %d: chan %d enabled=%d\n",
			th->tid, th->state, chan->id, enabled);

	chan_enable(chan, enabled);
}

/* Sets the state of the thread and updates the thread tracking channels */
static void
thread_set_state(struct ovni_ethread *th, enum ethread_state state)
{
	int i;

	/* The state must be updated when a cpu is set */
	assert(th->cpu);

	dbg("thread_set_state: setting thread %d state %d\n",
			th->tid, state);

	th->state = state;

	th->is_running = (state == TH_ST_RUNNING) ? 1 : 0;

	th->is_active = (state == TH_ST_RUNNING
			|| state == TH_ST_COOLING
			|| state == TH_ST_WARMING) ? 1 : 0;

	chan_set(&th->chan[CHAN_OVNI_STATE], th->state);
	
	/* Enable or disable the thread channels that track the thread state */
	for(i=0; i<CHAN_MAX; i++)
		chan_tracking_update(&th->chan[i], th);

	dbg("thread_set_state: done\n");
}

static void
cpu_update_th_stats(struct ovni_cpu *cpu)
{
	struct ovni_ethread *th, *th_running = NULL, *th_active = NULL;
	int active = 0, running = 0;

	DL_FOREACH(cpu->thread, th)
	{
		if(th->state == TH_ST_RUNNING)
		{
			th_running = th;
			running++;
			th_active = th;
			active++;
		}
		else if(th->state == TH_ST_COOLING || th->state == TH_ST_WARMING)
		{
			th_active = th;
			active++;
		}
	}

	cpu->nrunning_threads = running;
	cpu->nactive_threads = active;
	cpu->th_running = th_running;
	cpu->th_active = th_active;
}

static void
update_cpu(struct ovni_cpu *cpu)
{
	int i;

	dbg("updating cpu %s\n", cpu->name);

	/* Update the running and active threads first */
	cpu_update_th_stats(cpu);

	/* From the CPU channels we only need to manually update the number of
	 * threads running in the CPU */
	if(chan_get_st(&cpu->chan[CHAN_OVNI_NRTHREADS]) != (int) cpu->nrunning_threads)
		chan_set(&cpu->chan[CHAN_OVNI_NRTHREADS], (int) cpu->nrunning_threads);

	/* Update all tracking channels */
	for(i=0; i<CHAN_MAX; i++)
		emu_cpu_update_chan(cpu, &cpu->chan[i]);

	dbg("updating cpu %s complete!\n", cpu->name);
}

struct ovni_cpu *
emu_get_cpu(struct ovni_loom *loom, int cpuid)
{
	assert(cpuid < (int) loom->ncpus);

	if(cpuid < 0)
	{
		return &loom->vcpu;
	}

	return &loom->cpu[cpuid];
}

static struct ovni_ethread *
emu_cpu_find_thread(struct ovni_cpu *cpu, struct ovni_ethread *thread)
{
	struct ovni_ethread *p = NULL;

	DL_FOREACH(cpu->thread, p)
	{
		if(p == thread)
			return p;
	}

	return NULL;
}

/* Add the given thread to the list of threads assigned to the CPU */
static void
cpu_add_thread(struct ovni_cpu *cpu, struct ovni_ethread *thread)
{
	/* Found, abort */
	if(emu_cpu_find_thread(cpu, thread) != NULL)
	{
		err("The thread %d is already assigned to %s\n",
				thread->tid, cpu->name);
		abort();
	}

	DL_APPEND(cpu->thread, thread);
	cpu->nthreads++;

	update_cpu(cpu);
}

static void
cpu_remove_thread(struct ovni_cpu *cpu, struct ovni_ethread *thread)
{
	struct ovni_ethread *p;

	p = emu_cpu_find_thread(cpu, thread);

	/* Not found, abort */
	if(p == NULL)
	{
		err("cannot remove missing thread %d from cpu %s\n",
				thread->tid, cpu->name);
		abort();
	}

	DL_DELETE(cpu->thread, thread);
	cpu->nthreads--;

	update_cpu(cpu);
}

static void
cpu_migrate_thread(struct ovni_cpu *cpu,
		struct ovni_ethread *thread,
		struct ovni_cpu *newcpu)
{

	cpu_remove_thread(cpu, thread);
	cpu_add_thread(newcpu, thread);
}

/* Sets the thread assigned CPU to the given one.
 * Precondition: the thread CPU must be null */
static void
thread_set_cpu(struct ovni_ethread *th, struct ovni_cpu *cpu)
{
	assert(th->cpu == NULL);
	dbg("thread_set_cpu: setting thread %d cpu to %s\n",
			th->tid, cpu->name);
	th->cpu = cpu;

	chan_enable(&th->chan[CHAN_OVNI_CPU], 1);
	chan_set(&th->chan[CHAN_OVNI_CPU], cpu->gindex + 1);
}

/* Unsets the thread assigned CPU.
 * Precondition: the thread CPU must be not null */
static void
thread_unset_cpu(struct ovni_ethread *th)
{
	assert(th->cpu != NULL);
	th->cpu = NULL;

	chan_enable(&th->chan[CHAN_OVNI_CPU], 0);
}

/* Migrates the thread assigned CPU to the given one.
 * Precondition: the thread CPU must be not null */
static void
thread_migrate_cpu(struct ovni_ethread *th, struct ovni_cpu *cpu)
{
	assert(th->cpu != NULL);
	th->cpu = cpu;

	assert(chan_is_enabled(&th->chan[CHAN_OVNI_CPU]));
	chan_set(&th->chan[CHAN_OVNI_CPU], cpu->gindex + 1);
}

static void
pre_thread_execute(struct ovni_emu *emu, struct ovni_ethread *th)
{
	struct ovni_cpu *cpu;
	int cpuid;

	/* The thread cannot be already running */
	assert(th->state != TH_ST_RUNNING);

	cpuid = emu->cur_ev->payload.i32[0];

	cpu = emu_get_cpu(emu->cur_loom, cpuid);
	dbg("pre_thread_execute: thread %d runs in CPU %s\n", th->tid, cpu->name);

	/* First set the CPU in the thread */
	thread_set_cpu(th, cpu);

	/* Then set the thread to running state */
	thread_set_state(th, TH_ST_RUNNING);

	/* And then add the thread to the CPU, so tracking channels see the
	 * updated thread state */
	cpu_add_thread(cpu, th);
}

static void
pre_thread_end(struct ovni_ethread *th)
{
	assert(th->state == TH_ST_RUNNING);
	assert(th->cpu);

	/* First update the thread state */
	thread_set_state(th, TH_ST_DEAD);

	/* Then remove it from the cpu, so channels are properly updated */
	cpu_remove_thread(th->cpu, th);

	thread_unset_cpu(th);
}

static void
pre_thread_pause(struct ovni_ethread *th)
{
	assert(th->state == TH_ST_RUNNING || th->state == TH_ST_COOLING);
	assert(th->cpu);

	thread_set_state(th, TH_ST_PAUSED);
	update_cpu(th->cpu);
}

static void
pre_thread_resume(struct ovni_ethread *th)
{
	assert(th->state == TH_ST_PAUSED || th->state == TH_ST_WARMING);
	assert(th->cpu);

	thread_set_state(th, TH_ST_RUNNING);
	update_cpu(th->cpu);
}

static void
pre_thread_cool(struct ovni_ethread *th)
{
	assert(th->state == TH_ST_RUNNING);
	assert(th->cpu);

	thread_set_state(th, TH_ST_COOLING);
	update_cpu(th->cpu);
}

static void
pre_thread_warm(struct ovni_ethread *th)
{
	assert(th->state == TH_ST_PAUSED);
	assert(th->cpu);

	thread_set_state(th, TH_ST_WARMING);
	update_cpu(th->cpu);
}

static void
pre_thread(struct ovni_emu *emu)
{
	struct ovni_ev *ev;
	struct ovni_ethread *th;

	//emu_emit(emu);

	th = emu->cur_thread;
	ev = emu->cur_ev;

	switch(ev->header.value)
	{
		case 'C': /* create */
			dbg("thread %d creates a new thread at cpu=%d with args=%x %x\n",
					th->tid,
					ev->payload.u32[0],
					ev->payload.u32[1],
					ev->payload.u32[2]);

			break;
		case 'x': pre_thread_execute(emu, th); break;
		case 'e': pre_thread_end(th); break;
		case 'p': pre_thread_pause(th); break;
		case 'r': pre_thread_resume(th); break;
		case 'c': pre_thread_cool(th); break;
		case 'w': pre_thread_warm(th); break;
		default:
			  err("unknown thread event value %c\n",
					  ev->header.value);
			  exit(EXIT_FAILURE);
	}
}

static void
pre_affinity_set(struct ovni_emu *emu)
{
	int cpuid;
	struct ovni_cpu *newcpu;
	struct ovni_ethread *th;

	th = emu->cur_thread;
	cpuid = emu->cur_ev->payload.i32[0];

	assert(th->cpu);
	assert(th->state == TH_ST_RUNNING
			|| th->state == TH_ST_COOLING
			|| th->state == TH_ST_WARMING);

	/* Migrate current cpu to the one at cpuid */
	newcpu = emu_get_cpu(emu->cur_loom, cpuid);

	/* The CPU is already properly set, return */
	if(th->cpu == newcpu)
		return;

	cpu_migrate_thread(th->cpu, th, newcpu);
	thread_migrate_cpu(th, newcpu);

	//dbg("cpu %d now runs %d\n", cpuid, th->tid);
}

static void
pre_affinity_remote(struct ovni_emu *emu)
{
	size_t i;
	int32_t cpuid, tid;
	struct ovni_cpu *newcpu;
	struct ovni_ethread *remote_th;
	struct ovni_loom *loom;
	struct ovni_eproc *proc;

	cpuid = emu->cur_ev->payload.i32[0];
	tid = emu->cur_ev->payload.i32[1];

	remote_th = emu_get_thread(emu->cur_proc, tid);

	if(remote_th == NULL)
	{
		/* Search the thread in other processes of the loom if
		 * not found in the current one */
		loom = emu->cur_loom;

		for(i=0; i<loom->nprocs; i++)
		{
			proc = &loom->proc[i];

			/* Skip the current process */
			if(proc == emu->cur_proc)
				continue;

			remote_th = emu_get_thread(proc, tid);

			if(remote_th)
				break;
		}

		if(remote_th == NULL)
		{
			err("thread tid %d not found: cannot set affinity remotely\n",
					tid);
			abort();
		}
	}

	/* The remote_th cannot be in states dead or unknown */
	assert(remote_th->state != TH_ST_DEAD
			&& remote_th->state != TH_ST_UNKNOWN);

	/* It must have an assigned CPU */
	assert(remote_th->cpu);

	/* Migrate current cpu to the one at cpuid */
	newcpu = emu_get_cpu(emu->cur_loom, cpuid);

	cpu_migrate_thread(remote_th->cpu, remote_th, newcpu);
	thread_migrate_cpu(remote_th, newcpu);

	//dbg("remote_th %d switches to cpu %d by remote petition\n", tid,
	//		cpuid);
}

static void
pre_affinity(struct ovni_emu *emu)
{
	//emu_emit(emu);
	switch(emu->cur_ev->header.value)
	{
		case 's': pre_affinity_set(emu); break;
		case 'r': pre_affinity_remote(emu); break;
		default:
			dbg("unknown affinity event value %c\n",
					emu->cur_ev->header.value);
			break;
	}
}

static void
pre_burst(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	int64_t dt = 0;

	UNUSED(dt);

	th = emu->cur_thread;

	if(th->nbursts >= MAX_BURSTS)
	{
		err("too many bursts: ignored\n");
		return;
	}

	th->burst_time[th->nbursts] = emu->delta_time;
	if(th->nbursts > 0)
	{
		dt = th->burst_time[th->nbursts] -
			th->burst_time[th->nbursts - 1];

		dbg("burst delta time %ld ns\n", dt);
	}

	th->nbursts++;
}

static void
pre_flush(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_OVNI_FLUSH];

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(chan_th, ST_OVNI_FLUSHING);
			break;
		case ']':
			chan_pop(chan_th, ST_OVNI_FLUSHING);
			break;
		default:
			err("unexpected value '%c' (expecting '[' or ']')\n",
					emu->cur_ev->header.value);
			abort();
	}
}

void
hook_pre_ovni(struct ovni_emu *emu)
{
	//emu_emit(emu);

	if(emu->cur_ev->header.model != 'O')
		return;

	switch(emu->cur_ev->header.category)
	{
		case 'H': pre_thread(emu); break;
		case 'A': pre_affinity(emu); break;
		case 'B': pre_burst(emu); break;
		case 'F': pre_flush(emu); break;
		default:
			dbg("unknown ovni event category %c\n",
					emu->cur_ev->header.category);
			break;
	}
}
