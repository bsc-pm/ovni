/*
 * Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
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

#include "uthash.h"
#include "utlist.h"

#include "ovni.h"
#include "emu.h"
#include "emu_task.h"
#include "prv.h"
#include "chan.h"

void
hook_init_nanos6(struct ovni_emu *emu)
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

	/* Init the channels in all threads */
	for(i=0; i<emu->total_nthreads; i++)
	{
		th = emu->global_thread[i];
		row = th->gindex + 1;

		uth = &emu->th_chan;

		chan_th_init(th, uth, CHAN_NANOS6_TASKID,    CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NANOS6_TYPE,      CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NANOS6_SUBSYSTEM, CHAN_TRACK_TH_ACTIVE,  0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NANOS6_RANK,      CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
	}

	/* Init the Nanos6 channels in all cpus */
	for(i=0; i<emu->total_ncpus; i++)
	{
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;
		ucpu = &emu->cpu_chan;

		chan_cpu_init(cpu, ucpu, CHAN_NANOS6_TASKID,    CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NANOS6_TYPE,      CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NANOS6_SUBSYSTEM, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NANOS6_RANK,      CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
	}
}

/* --------------------------- pre ------------------------------- */

static void
task_not_running(struct ovni_emu *emu, struct task *task, enum nanos6_task_run_reason reason)
{
	struct ovni_ethread *th;
	th = emu->cur_thread;

	if(task->state == TASK_ST_RUNNING)
		die("task is still running\n");

	chan_set(&th->chan[CHAN_NANOS6_TASKID], 0);
	chan_set(&th->chan[CHAN_NANOS6_TYPE], 0);

	if(emu->cur_loom->rank_enabled)
		chan_set(&th->chan[CHAN_NANOS6_RANK], 0);

	// Check the reason
	switch (reason)
	{
		case TB_EXEC_OR_END:
			chan_pop(&th->chan[CHAN_NANOS6_SUBSYSTEM], ST_NANOS6_TASK_RUNNING);
			break;
		case TB_BLOCKING_API:
			chan_push(&th->chan[CHAN_NANOS6_SUBSYSTEM], ST_NANOS6_BLOCKING);
			break;
		case TB_TASKWAIT:
			chan_push(&th->chan[CHAN_NANOS6_SUBSYSTEM], ST_NANOS6_TASKWAIT);
			break;
		case TB_WAITFOR:
			chan_push(&th->chan[CHAN_NANOS6_SUBSYSTEM], ST_NANOS6_WAITFOR);
			break;
	}
}

static void
task_running(struct ovni_emu *emu, struct task *task, enum nanos6_task_run_reason reason)
{
	struct ovni_ethread *th;
	struct ovni_eproc *proc;

	th = emu->cur_thread;
	proc = emu->cur_proc;

	if(task->id == 0)
		die("task id cannot be 0\n");

	if(task->type->gid == 0)
		die("task type gid cannot be 0\n");

	if(proc->appid <= 0)
		die("app id must be positive\n");

	chan_set(&th->chan[CHAN_NANOS6_TASKID], task->id);
	chan_set(&th->chan[CHAN_NANOS6_TYPE], task->type->gid);

	if(emu->cur_loom->rank_enabled)
		chan_set(&th->chan[CHAN_NANOS6_RANK], proc->rank + 1);

	// Check the reason
	switch (reason)
	{
		case TB_EXEC_OR_END:
			chan_push(&th->chan[CHAN_NANOS6_SUBSYSTEM], ST_NANOS6_TASK_RUNNING);
			break;
		case TB_BLOCKING_API:
			chan_pop(&th->chan[CHAN_NANOS6_SUBSYSTEM], ST_NANOS6_BLOCKING);
			break;
		case TB_TASKWAIT:
			chan_pop(&th->chan[CHAN_NANOS6_SUBSYSTEM], ST_NANOS6_TASKWAIT);
			break;
		case TB_WAITFOR:
			chan_pop(&th->chan[CHAN_NANOS6_SUBSYSTEM], ST_NANOS6_WAITFOR);
			break;
	}
}

static void
task_switch(struct ovni_emu *emu, struct task *prev_task,
		struct task *next_task, int newtask)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;

	if(!prev_task || !next_task)
		die("cannot switch to or from a NULL task\n");

	if(prev_task == next_task)
		die("cannot switch to the same task\n");

	if(newtask && prev_task->state != TASK_ST_RUNNING)
		die("previous task must not be no longer running\n");

	if(!newtask && prev_task->state != TASK_ST_DEAD)
		die("previous task must be dead\n");

	if(next_task->state != TASK_ST_RUNNING)
		die("next task must be running\n");

	if(next_task->id == 0)
		die("next task id cannot be 0\n");

	if(next_task->type->gid == 0)
		die("next task type id cannot be 0\n");

	if(prev_task->thread != next_task->thread)
		die("cannot switch to a task of another thread\n");

	/* No need to change the rank as we will switch to tasks from same stack */
	chan_set(&th->chan[CHAN_NANOS6_TASKID], next_task->id);

	/* FIXME: We should emit a PRV event even if we are switching to
	 * the same type event, to mark the end of the current task. For
	 * now we only emit a new type if we switch to a type with a
	 * different gid. */
	if(prev_task->type->gid != next_task->type->gid)
		chan_set(&th->chan[CHAN_NANOS6_TYPE], next_task->type->gid);
}

static void
pre_task(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;
	struct task **task_map = &emu->cur_proc->nanos6_tasks;
	struct task_type **type_map = &emu->cur_proc->nanos6_types;
	struct task **task_stack = &emu->cur_thread->nanos6_task_stack;
	struct task *prev_running = task_get_running(*task_stack);
	int was_running_task = (prev_running != NULL);

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	/* Update the emulator state, but don't modify the channels yet */
	switch(emu->cur_ev->header.value)
	{
		case 'c': task_create(emu->cur_ev->payload.i32[0], emu->cur_ev->payload.i32[1], task_map, type_map); break;
		case 'x': task_execute(emu->cur_ev->payload.i32[0], emu->cur_thread, task_map, task_stack); break;
		case 'e': task_end(emu->cur_ev->payload.i32[0], emu->cur_thread, task_map, task_stack); break;
		case 'b': task_pause(emu->cur_ev->payload.i32[0], emu->cur_thread, task_map, task_stack); break;
		case 'u': task_resume(emu->cur_ev->payload.i32[0], emu->cur_thread, task_map, task_stack); break;
		case 'C': break;
		default:
			  abort();
	}

	struct task *next_running = task_get_running(*task_stack);
	int runs_task_now = (next_running != NULL);

	/* Now that we know if the emulator was running a task before
	 * or if it's running one now, update the channels accordingly. */
	switch(emu->cur_ev->header.value)
	{
		case 'x': /* Execute: either a nested task or a new one */
			if(was_running_task)
				task_switch(emu, prev_running, next_running, 1);
			else
				task_running(emu, next_running, TB_EXEC_OR_END);
			break;
		case 'e': /* End: either a nested task or the last one */
			if(runs_task_now)
				task_switch(emu, prev_running, next_running, 0);
			else
				task_not_running(emu, prev_running, TB_EXEC_OR_END);
			break;
		case 'b': /* Block */
			task_not_running(emu, prev_running, emu->cur_ev->payload.i32[1]);
			break;
		case 'u': /* Unblock */
			task_running(emu, next_running, emu->cur_ev->payload.i32[1]);
			break;
		case 'c': /* Create */
			chan_push(chan_th, ST_NANOS6_CREATING);
			break;
		case 'C': /* Create end */
			chan_pop(chan_th, ST_NANOS6_CREATING);
			break;
		default:
			break;
	}
}

static void
pre_type(struct ovni_emu *emu)
{
	uint8_t *data;

	switch(emu->cur_ev->header.value)
	{
		case 'c':
			if((emu->cur_ev->header.flags & OVNI_EV_JUMBO) == 0)
			{
				err("expecting a jumbo event\n");
				abort();
			}

			data = &emu->cur_ev->payload.jumbo.data[0];
			uint32_t *typeid = (uint32_t *) data;
			data += sizeof(*typeid);
			const char *label = (const char *) data;
			task_type_create(*typeid, label, &emu->cur_proc->nanos6_types);
			break;
		default:
			  break;
	}
}

static void
pre_deps(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case 'r': chan_push(chan_th, ST_NANOS6_DEP_REG); break;
		case 'R': chan_pop(chan_th, ST_NANOS6_DEP_REG); break;
		case 'u': chan_push(chan_th, ST_NANOS6_DEP_UNREG); break;
		case 'U': chan_pop(chan_th, ST_NANOS6_DEP_UNREG); break;
		default: break;
	}
}

static void
pre_blocking(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case 'u': chan_push(chan_th, ST_NANOS6_UNBLOCKING); break;
		case 'U': chan_pop(chan_th, ST_NANOS6_UNBLOCKING); break;
		default: break;
	}
}

static void
pre_sched(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case 'h': chan_push(chan_th, ST_NANOS6_SCHED_HUNGRY); break;
		case 'f': chan_pop(chan_th, ST_NANOS6_SCHED_HUNGRY); break;
		case '[': chan_push(chan_th, ST_NANOS6_SCHED_SERVING); break;
		case ']': chan_pop(chan_th, ST_NANOS6_SCHED_SERVING); break;
		case '@': chan_ev(chan_th, EV_NANOS6_SCHED_SELF); break;
		case 'r': chan_ev(chan_th, EV_NANOS6_SCHED_RECV); break;
		case 's': chan_ev(chan_th, EV_NANOS6_SCHED_SEND); break;
		default: break;
	}
}

static void
pre_thread_type(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case 'a': chan_push(chan_th, ST_NANOS6_ATTACHED); break;
		case 'A': chan_pop (chan_th, ST_NANOS6_ATTACHED); break;
		case 's': chan_push(chan_th, ST_NANOS6_SPAWNING); break;
		case 'S': chan_pop (chan_th, ST_NANOS6_SPAWNING); break;
		default: break;
	}
}

static void
pre_ss(struct ovni_emu *emu, int st)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	dbg("pre_ss chan id %d st=%d\n", chan_th->id, st);

	switch(emu->cur_ev->header.value)
	{
		case '[': chan_push(chan_th, st); break;
		case ']': chan_pop(chan_th, st); break;
		default:
			err("unexpected value '%c' (expecting '[' or ']')\n",
					emu->cur_ev->header.value);
			abort();
	}
}

static void
check_affinity(struct ovni_emu *emu)
{
	struct ovni_ethread *th = emu->cur_thread;
	struct ovni_cpu *cpu = th->cpu;

	if(!cpu || cpu->virtual)
		return;

	if(cpu->nrunning_threads > 1)
	{
		die("cpu %s has more than one thread running\n",
				cpu->name);
	}
}

void
hook_pre_nanos6(struct ovni_emu *emu)
{
	if(emu->cur_ev->header.model != '6')
		die("hook_pre_nanos6: unexpected event with model %c\n",
				emu->cur_ev->header.model);

	if(!emu->cur_thread->is_active)
		die("hook_pre_nanos6: current thread %d not active\n",
				emu->cur_thread->tid);

	switch(emu->cur_ev->header.category)
	{
		case 'T': pre_task(emu); break;
		case 'Y': pre_type(emu); break;
		case 'S': pre_sched(emu); break;
		case 'U': pre_ss(emu, ST_NANOS6_SUBMIT); break;
		case 'H': pre_thread_type(emu); break;
		case 'D': pre_deps(emu); break;
		case 'B': pre_blocking(emu); break;
		default:
			break;
	}

	check_affinity(emu);
}

static void
end_lint(struct ovni_emu *emu)
{
	/* Ensure we run out of subsystem states */
	for(size_t i = 0; i < emu->total_nthreads; i++)
	{
		struct ovni_ethread *th = emu->global_thread[i];
		struct ovni_chan *ch = &th->chan[CHAN_NANOS6_SUBSYSTEM];
		if(ch->n != 1)
		{
			int top = ch->stack[ch->n - 1];
			die("thread %ld has left %d state(s) in the subsystem channel, top state=%d\n",
					i, ch->n - 1, top);
		}
	}
}

void
hook_end_nanos6(struct ovni_emu *emu)
{
	/* Emit types for all channel types and processes */
	for(enum chan_type ct = 0; ct < CHAN_MAXTYPE; ct++)
	{
		struct pcf_file *pcf = &emu->pcf[ct];
		int typeid = chan_to_prvtype[CHAN_NANOS6_TYPE];
		struct pcf_type *pcftype = pcf_find_type(pcf, typeid);

		for(size_t i = 0; i < emu->trace.nlooms; i++)
		{
			struct ovni_loom *loom = &emu->trace.loom[i];
			for(size_t j = 0; j < loom->nprocs; j++)
			{
				struct ovni_eproc *proc = &loom->proc[j];
				task_create_pcf_types(pcftype, proc->nanos6_types);
			}
		}
	}

	/* When running in linter mode perform additional checks */
	if(emu->enable_linter)
		end_lint(emu);
}
