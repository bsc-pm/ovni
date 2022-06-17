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

#include "uthash.h"
#include "utlist.h"

#include "ovni.h"
#include "emu.h"
#include "prv.h"
#include "chan.h"

/* --------------------------- init ------------------------------- */

void
hook_init_nosv(struct ovni_emu *emu)
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

		chan_th_init(th, uth, CHAN_NOSV_TASKID, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NOSV_TYPE,   CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NOSV_APPID,  CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NOSV_RANK,   CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);

		/* We allow threads to emit subsystem events in cooling and
		 * warming states as well, as they may be allocating memory.
		 * However, these information won't be presented in the CPU
		 * channel, as it only shows the thread in the running state */
		chan_th_init(th, uth, CHAN_NOSV_SUBSYSTEM, CHAN_TRACK_TH_ACTIVE, 0, 0, 1, row, prv_th, clock);
	}

	/* Init the nosv channels in all cpus */
	for(i=0; i<emu->total_ncpus; i++)
	{
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;
		ucpu = &emu->cpu_chan;

		chan_cpu_init(cpu, ucpu, CHAN_NOSV_TASKID,    CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NOSV_TYPE,      CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NOSV_APPID,     CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NOSV_RANK,      CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NOSV_SUBSYSTEM, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
	}
}

/* --------------------------- pre ------------------------------- */

static void
pre_task_create(struct ovni_emu *emu)
{
	uint32_t task_id = emu->cur_ev->payload.u32[0];
	uint32_t type_id = emu->cur_ev->payload.u32[1];

	/* Ensure the task id is new */
	struct nosv_task *task = NULL;
	HASH_FIND_INT(emu->cur_proc->tasks, &task_id, task);

	if(task != NULL)
		die("a task with id %u already exists\n", task_id);

	/* Ensure the type exists */
	struct nosv_task_type *type = NULL;
	HASH_FIND_INT(emu->cur_proc->types, &type_id, type);

	if(type == NULL)
		die("unknown task type id %u\n", type_id);

	task = calloc(1, sizeof(*task));

	if(task == NULL)
		die("calloc failed\n");

	task->id = task_id;
	task->type = type;
	task->state = TASK_ST_CREATED;
	task->thread = NULL;

	/* Add the new task to the hash table */
	HASH_ADD_INT(emu->cur_proc->tasks, id, task);

	dbg("new task created id=%d\n", task->id);
}

static void
pre_task_execute(struct ovni_emu *emu)
{
	struct nosv_task *top = emu->cur_thread->task_stack;
	uint32_t taskid = emu->cur_ev->payload.u32[0];

	struct nosv_task *task = NULL;
	HASH_FIND_INT(emu->cur_proc->tasks, &taskid, task);

	if(task == NULL)
		die("cannot find task with id %u\n", taskid);

	if(task->state != TASK_ST_CREATED)
		die("task state is not created\n");

	if(task->thread != NULL)
		die("task already has a thread assigned\n");

	if(emu->cur_thread->state != TH_ST_RUNNING)
		die("thread state is not running\n");

	if(top == task)
		die("thread already has assigned task %u\n", taskid);

	if(top && top->state != TASK_ST_RUNNING)
		die("cannot execute a nested task from a non-running task\n");

	task->state = TASK_ST_RUNNING;
	task->thread = emu->cur_thread;

	DL_PREPEND(emu->cur_thread->task_stack, task);

	dbg("task id=%u runs now\n", task->id);
}

static void
pre_task_pause(struct ovni_emu *emu)
{
	struct nosv_task *top = emu->cur_thread->task_stack;
	uint32_t taskid = emu->cur_ev->payload.u32[0];

	struct nosv_task *task = NULL;
	HASH_FIND_INT(emu->cur_proc->tasks, &taskid, task);

	if(task == NULL)
		die("cannot find task with id %u\n", taskid);

	if(task->state != TASK_ST_RUNNING)
		die("task state is not running\n");

	if(emu->cur_thread->state != TH_ST_RUNNING)
		die("thread state is not running\n");

	if(top != task)
		die("thread has assigned a different task\n");

	if(emu->cur_thread != task->thread)
		die("task is assigned to a different thread\n");

	task->state = TASK_ST_PAUSED;

	dbg("task id=%d pauses\n", task->id);
}

static void
pre_task_resume(struct ovni_emu *emu)
{
	struct nosv_task *top = emu->cur_thread->task_stack;
	uint32_t taskid = emu->cur_ev->payload.u32[0];

	struct nosv_task *task = NULL;
	HASH_FIND_INT(emu->cur_proc->tasks, &taskid, task);

	if(task == NULL)
		die("cannot find task with id %u\n", taskid);

	if(task->state != TASK_ST_PAUSED)
		die("task state is not paused\n");

	if(emu->cur_thread->state != TH_ST_RUNNING)
		die("thread is not running\n");

	if(top != task)
		die("thread has assigned a different task\n");

	if(emu->cur_thread != task->thread)
		die("task is assigned to a different thread\n");

	task->state = TASK_ST_RUNNING;

	dbg("task id=%d resumes\n", task->id);
}

static void
pre_task_end(struct ovni_emu *emu)
{
	struct nosv_task *top = emu->cur_thread->task_stack;
	uint32_t taskid = emu->cur_ev->payload.u32[0];

	struct nosv_task *task = NULL;
	HASH_FIND_INT(emu->cur_proc->tasks, &taskid, task);

	if(task == NULL)
		die("cannot find task with id %u\n", taskid);

	if(task->state != TASK_ST_RUNNING)
		die("task state is not running\n");

	if(emu->cur_thread->state != TH_ST_RUNNING)
		die("thread is not running\n");

	if(top != task)
		die("thread has assigned a different task\n");

	if(emu->cur_thread != task->thread)
		die("task is assigned to a different thread\n");

	task->state = TASK_ST_DEAD;

	/* Don't unset the thread from the task, as it will be used
	 * later to ensure we switch to tasks of the same thread. */

	DL_DELETE(emu->cur_thread->task_stack, task);

	dbg("task id=%d ends\n", task->id);
}

static void
pre_task_not_running(struct ovni_emu *emu, struct nosv_task *task)
{
	struct ovni_ethread *th;
	th = emu->cur_thread;

	if(task->state == TASK_ST_RUNNING)
		die("task is still running\n");

	chan_set(&th->chan[CHAN_NOSV_TASKID], 0);
	chan_set(&th->chan[CHAN_NOSV_TYPE], 0);
	chan_set(&th->chan[CHAN_NOSV_APPID], 0);

	if(emu->cur_loom->rank_enabled)
		chan_set(&th->chan[CHAN_NOSV_RANK], 0);

	chan_pop(&th->chan[CHAN_NOSV_SUBSYSTEM], ST_NOSV_TASK_RUNNING);
}

static void
pre_task_running(struct ovni_emu *emu, struct nosv_task *task)
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

	chan_set(&th->chan[CHAN_NOSV_TASKID], task->id);
	chan_set(&th->chan[CHAN_NOSV_TYPE], task->type->gid);
	chan_set(&th->chan[CHAN_NOSV_APPID], proc->appid);

	if(emu->cur_loom->rank_enabled)
		chan_set(&th->chan[CHAN_NOSV_RANK], proc->rank + 1);

	chan_push(&th->chan[CHAN_NOSV_SUBSYSTEM], ST_NOSV_TASK_RUNNING);
}

static void
pre_task_switch(struct ovni_emu *emu, struct nosv_task *prev_task,
		struct nosv_task *next_task, int newtask)
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

	/* No need to change the rank or app ID, as we can only switch
	 * to tasks of the same thread */
	chan_set(&th->chan[CHAN_NOSV_TASKID], next_task->id);

	/* FIXME: We should emit a PRV event even if we are switching to
	 * the same type event, to mark the end of the current task. For
	 * now we only emit a new type if we switch to a type with a
	 * different gid. */
	if(prev_task->type->gid != next_task->type->gid)
		chan_set(&th->chan[CHAN_NOSV_TYPE], next_task->type->gid);
}

static struct nosv_task *
get_running_task(struct ovni_emu *emu)
{
	struct nosv_task *task = emu->cur_thread->task_stack;
	if(task && task->state == TASK_ST_RUNNING)
		return task;

	return NULL;
}

static void
pre_task(struct ovni_emu *emu)
{
	struct nosv_task *prev_running = get_running_task(emu);
	int was_running_task = (prev_running != NULL);

	/* Update the emulator state, but don't modify the channels yet */
	switch(emu->cur_ev->header.value)
	{
		case 'c': pre_task_create(emu); break;
		case 'x': pre_task_execute(emu); break;
		case 'e': pre_task_end(emu); break;
		case 'p': pre_task_pause(emu); break;
		case 'r': pre_task_resume(emu); break;
		default:
			  abort();
	}

	struct nosv_task *next_running = get_running_task(emu);
	int runs_task_now = (next_running != NULL);

	/* Now that we know if the emulator was running a task before
	 * or if it's running one now, update the channels accordingly. */
	switch(emu->cur_ev->header.value)
	{
		case 'x': /* Execute: either a nested task or a new one */
			if(was_running_task)
				pre_task_switch(emu, prev_running, next_running, 1);
			else
				pre_task_running(emu, next_running);
			break;
		case 'e': /* End: either a nested task or the last one */
			if(runs_task_now)
				pre_task_switch(emu, prev_running, next_running, 0);
			else
				pre_task_not_running(emu, prev_running);
			break;
		case 'p': /* Pause */
			pre_task_not_running(emu, prev_running);
			break;
		case 'r': /* Resume */
			pre_task_running(emu, next_running);
			break;
		default:
			break;
	}
}

static uint32_t
get_task_type_gid(const char *label)
{
	uint32_t gid;

	HASH_VALUE(label, strlen(label), gid);

	/* Use non-negative values */
	gid &= 0x7FFFFFFF;

	if (gid == 0)
		gid++;

	return gid;
}

static void
pre_type_create(struct ovni_emu *emu)
{
	struct nosv_task_type *type;
	uint8_t *data;
	uint32_t *typeid;
	const char *label;

	if((emu->cur_ev->header.flags & OVNI_EV_JUMBO) == 0)
	{
		err("expecting a jumbo event\n");
		abort();
	}

	data = &emu->cur_ev->payload.jumbo.data[0];
	typeid = (uint32_t *) data;
	data += sizeof(*typeid);
	label = (const char *) data;

	/* Ensure the type id is new */
	HASH_FIND_INT(emu->cur_proc->types, typeid, type);

	if(type != NULL)
	{
		err("A task type with id %d already exists\n", *typeid);
		abort();
	}

	type = calloc(1, sizeof(*type));

	if(type == NULL)
	{
		perror("calloc");
		abort();
	}

	type->id = *typeid;

	if(type->id == 0)
		die("invalid task type id %d\n", type->id);

	type->gid = get_task_type_gid(label);
	int n = snprintf(type->label, MAX_PCF_LABEL, "%s", label);
	if(n >= MAX_PCF_LABEL)
		die("task label too long: %s\n", label);

	/* Add the new task type to the hash table */
	HASH_ADD_INT(emu->cur_proc->types, id, type);

	dbg("new task type created id=%d label=%s\n", type->id,
			type->label);
}

static void
pre_type(struct ovni_emu *emu)
{
	switch(emu->cur_ev->header.value)
	{
		case 'c': pre_type_create(emu); break;
		default:
			  break;
	}
}

static void
pre_sched(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case 'h':
			chan_push(chan_th, ST_NOSV_SCHED_HUNGRY);
			break;
		case 'f': /* Fill: no longer hungry */
			chan_pop(chan_th, ST_NOSV_SCHED_HUNGRY);
			break;
		case '[': /* Server enter */
			chan_push(chan_th, ST_NOSV_SCHED_SERVING);
			break;
		case ']': /* Server exit */
			chan_pop(chan_th, ST_NOSV_SCHED_SERVING);
			break;
		case '@':
			chan_ev(chan_th, EV_NOSV_SCHED_SELF);
			break;
		case 'r':
			chan_ev(chan_th, EV_NOSV_SCHED_RECV);
			break;
		case 's':
			chan_ev(chan_th, EV_NOSV_SCHED_SEND);
			break;
		default:
			break;
	}
}

static void
pre_api(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case 's': chan_push(chan_th, ST_NOSV_API_SUBMIT); break;
		case 'S': chan_pop (chan_th, ST_NOSV_API_SUBMIT); break;
		case 'p': chan_push(chan_th, ST_NOSV_API_PAUSE); break;
		case 'P': chan_pop (chan_th, ST_NOSV_API_PAUSE); break;
		case 'y': chan_push(chan_th, ST_NOSV_API_YIELD); break;
		case 'Y': chan_pop (chan_th, ST_NOSV_API_YIELD); break;
		case 'w': chan_push(chan_th, ST_NOSV_API_WAITFOR); break;
		case 'W': chan_pop (chan_th, ST_NOSV_API_WAITFOR); break;
		case 'c': chan_push(chan_th, ST_NOSV_API_SCHEDPOINT); break;
		case 'C': chan_pop (chan_th, ST_NOSV_API_SCHEDPOINT); break;
		default: break;
	}
}

static void
pre_mem(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case 'a': chan_push(chan_th, ST_NOSV_MEM_ALLOCATING); break;
		case 'A': chan_pop (chan_th, ST_NOSV_MEM_ALLOCATING); break;
		case 'f': chan_push(chan_th, ST_NOSV_MEM_FREEING); break;
		case 'F': chan_pop (chan_th, ST_NOSV_MEM_FREEING); break;
		default: break;
	}
}

static void
pre_thread_type(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case 'a': chan_push(chan_th, ST_NOSV_ATTACH); break;
		case 'A': chan_pop (chan_th, ST_NOSV_ATTACH); break;
		case 'w': chan_push(chan_th, ST_NOSV_WORKER); break;
		case 'W': chan_pop (chan_th, ST_NOSV_WORKER); break;
		case 'd': chan_push(chan_th, ST_NOSV_DELEGATE); break;
		case 'D': chan_pop (chan_th, ST_NOSV_DELEGATE); break;
		default: break;
	}
}

static void
pre_ss(struct ovni_emu *emu, int st)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NOSV_SUBSYSTEM];

	dbg("pre_ss chan id %d st=%d\n", chan_th->id, st);

	switch(emu->cur_ev->header.value)
	{
		case '[':
			chan_push(chan_th, st);
			break;
		case ']':
			chan_pop(chan_th, st);
			break;
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

	if(th->state != TH_ST_RUNNING)
		return;

	if(cpu->nrunning_threads > 1)
	{
		err("cpu %s has more than one thread running\n",
				cpu->name);
		abort();
	}
}

void
hook_pre_nosv(struct ovni_emu *emu)
{
	if(emu->cur_ev->header.model != 'V')
		die("hook_pre_nosv: unexpected event with model %c\n",
				emu->cur_ev->header.model);

	if(!emu->cur_thread->is_active)
		die("hook_pre_nosv: current thread %d not active\n",
				emu->cur_thread->tid);

	switch(emu->cur_ev->header.category)
	{
		case 'T': pre_task(emu); break;
		case 'Y': pre_type(emu); break;
		case 'S': pre_sched(emu); break;
		case 'U': pre_ss(emu, ST_NOSV_SCHED_SUBMITTING); break;
		case 'M': pre_mem(emu); break;
		case 'H': pre_thread_type(emu); break;
		case 'A': pre_api(emu); break;
		default:
			break;
	}

	if(emu->enable_linter)
		check_affinity(emu);
}

static void
create_pcf_task_types(struct ovni_eproc *proc, struct pcf_type *pcftype)
{
	/* Emit types for all task types */
	struct nosv_task_type *tt;
	for(tt = proc->types; tt != NULL; tt=tt->hh.next)
	{
		struct pcf_value *pcfvalue = pcf_find_value(pcftype, tt->gid);
		if(pcfvalue != NULL)
		{
			/* Ensure the label is the same, so we know that
			 * no collision occurred */
			if(strcmp(pcfvalue->label, tt->label) != 0)
				die("collision occurred in task type labels\n");
			else
				continue;
		}

		pcf_add_value(pcftype, tt->gid, tt->label);
	}
}

void
hook_end_nosv(struct ovni_emu *emu)
{
	/* Emit types for all channel types and processes */
	for(enum chan_type ct = 0; ct < CHAN_MAXTYPE; ct++)
	{
		struct pcf_file *pcf = &emu->pcf[ct];
		int typeid = chan_to_prvtype[CHAN_NOSV_TYPE][ct];
		struct pcf_type *pcftype = pcf_find_type(pcf, typeid);

		for(size_t i = 0; i < emu->trace.nlooms; i++)
		{
			struct ovni_loom *loom = &emu->trace.loom[i];
			for(size_t j = 0; j < loom->nprocs; j++)
			{
				struct ovni_eproc *proc = &loom->proc[j];
				create_pcf_task_types(proc, pcftype);
			}
		}
	}
}
