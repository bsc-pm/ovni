/* Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "uthash.h"
#include "utlist.h"

#include "chan.h"
#include "emu.h"
#include "emu_task.h"
#include "ovni.h"
#include "prv.h"

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
	for (i = 0; i < emu->total_nthreads; i++) {
		th = emu->global_thread[i];
		row = th->gindex + 1;

		uth = &emu->th_chan;

		chan_th_init(th, uth, CHAN_NOSV_TASKID, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NOSV_TYPE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NOSV_APPID, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NOSV_RANK, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);

		/* We allow threads to emit subsystem events in cooling and
		 * warming states as well, as they may be allocating memory.
		 * However, these information won't be presented in the CPU
		 * channel, as it only shows the thread in the running state */
		chan_th_init(th, uth, CHAN_NOSV_SUBSYSTEM, CHAN_TRACK_TH_ACTIVE, 0, 0, 1, row, prv_th, clock);
	}

	/* Init the nosv channels in all cpus */
	for (i = 0; i < emu->total_ncpus; i++) {
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;
		ucpu = &emu->cpu_chan;

		chan_cpu_init(cpu, ucpu, CHAN_NOSV_TASKID, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NOSV_TYPE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NOSV_APPID, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NOSV_RANK, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NOSV_SUBSYSTEM, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
	}

	/* Init task stack */
	for (i = 0; i < emu->total_nthreads; i++) {
		th = emu->global_thread[i];
		th->nosv_task_stack.thread = th;
	}
}

/* --------------------------- pre ------------------------------- */

static void
chan_task_stopped(struct ovni_emu *emu)
{
	struct ovni_ethread *th = emu->cur_thread;

	chan_set(&th->chan[CHAN_NOSV_TASKID], 0);
	chan_set(&th->chan[CHAN_NOSV_TYPE], 0);
	chan_set(&th->chan[CHAN_NOSV_APPID], 0);

	if (emu->cur_loom->rank_enabled)
		chan_set(&th->chan[CHAN_NOSV_RANK], 0);

	/* XXX: Do we need this transition? */
	chan_pop(&th->chan[CHAN_NOSV_SUBSYSTEM], ST_NOSV_TASK_RUNNING);
}

static void
chan_task_running(struct ovni_emu *emu, struct task *task)
{
	struct ovni_ethread *th;
	struct ovni_eproc *proc;

	th = emu->cur_thread;
	proc = emu->cur_proc;

	if (task->id == 0)
		edie(emu, "task id cannot be 0\n");

	if (task->type->gid == 0)
		edie(emu, "task type gid cannot be 0\n");

	if (proc->appid <= 0)
		edie(emu, "app id must be positive\n");

	chan_set(&th->chan[CHAN_NOSV_TASKID], task->id);
	chan_set(&th->chan[CHAN_NOSV_TYPE], task->type->gid);
	chan_set(&th->chan[CHAN_NOSV_APPID], proc->appid);

	if (emu->cur_loom->rank_enabled)
		chan_set(&th->chan[CHAN_NOSV_RANK], proc->rank + 1);

	chan_push(&th->chan[CHAN_NOSV_SUBSYSTEM], ST_NOSV_TASK_RUNNING);
}

static void
chan_task_switch(struct ovni_emu *emu,
	struct task *prev, struct task *next)
{
	struct ovni_ethread *th = emu->cur_thread;

	if (!prev || !next)
		edie(emu, "cannot switch to or from a NULL task\n");

	if (prev == next)
		edie(emu, "cannot switch to the same task\n");

	if (next->id == 0)
		edie(emu, "next task id cannot be 0\n");

	if (next->type->gid == 0)
		edie(emu, "next task type id cannot be 0\n");

	if (prev->thread != next->thread)
		edie(emu, "cannot switch to a task of another thread\n");

	/* No need to change the rank or app ID, as we can only switch
	 * to tasks of the same thread */
	chan_set(&th->chan[CHAN_NOSV_TASKID], next->id);

	/* FIXME: We should emit a PRV event even if we are switching to
	 * the same type event, to mark the end of the current task. For
	 * now we only emit a new type if we switch to a type with a
	 * different gid. */
	if (prev->type->gid != next->type->gid)
		chan_set(&th->chan[CHAN_NOSV_TYPE], next->type->gid);
}

static void
update_task_state(struct ovni_emu *emu)
{
	if (ovni_payload_size(emu->cur_ev) < 4)
		edie(emu, "missing task id in payload\n");

	uint32_t task_id = emu->cur_ev->payload.u32[0];

	struct ovni_ethread *th = emu->cur_thread;
	struct ovni_eproc *proc = emu->cur_proc;

	struct task_info *info = &proc->nosv_task_info;
	struct task_stack *stack = &th->nosv_task_stack;

	struct task *task = task_find(info->tasks, task_id);

	if (task == NULL)
		edie(emu, "cannot find task with id %u\n", task_id);

	switch (emu->cur_ev->header.value) {
		case 'x':
			task_execute(emu, stack, task);
			break;
		case 'e':
			task_end(emu, stack, task);
			break;
		case 'p':
			task_pause(emu, stack, task);
			break;
		case 'r':
			task_resume(emu, stack, task);
			break;
		default:
			edie(emu, "unexpected Nanos6 task event value %c\n",
				emu->cur_ev->header.value);
	}
}

static char
expand_transition_value(struct ovni_emu *emu, int was_running, int runs_now)
{
	char tr = emu->cur_ev->header.value;

	/* Ensure we don't clobber the value */
	if (tr == 'X' || tr == 'E')
		edie(emu, "unexpected event value %c\n", tr);

	/* Modify the event value to detect nested transitions */
	if (tr == 'x' && was_running)
		tr = 'X'; /* Execute a new nested task */
	else if (tr == 'e' && runs_now)
		tr = 'E'; /* End a nested task */

	return tr;
}

static void
update_task_channels(struct ovni_emu *emu,
	char tr, struct task *prev, struct task *next)
{
	switch (tr) {
		case 'x':
			chan_task_running(emu, next);
			break;
		case 'r':
			chan_task_running(emu, next);
			break;
		case 'e':
			chan_task_stopped(emu);
			break;
		case 'p':
			chan_task_stopped(emu);
			break;
		/* Additional nested transitions */
		case 'X':
			chan_task_switch(emu, prev, next);
			break;
		case 'E':
			chan_task_switch(emu, prev, next);
			break;
		default:
			edie(emu, "unexpected transition value %c\n", tr);
	}
}

static void
update_task(struct ovni_emu *emu)
{
	struct ovni_ethread *th = emu->cur_thread;
	struct task_stack *stack = &th->nosv_task_stack;

	struct task *prev = task_get_running(stack);

	/* Update the emulator state, but don't modify the channels */
	update_task_state(emu);

	struct task *next = task_get_running(stack);

	int was_running = (prev != NULL);
	int runs_now = (next != NULL);
	char tr = expand_transition_value(emu, was_running, runs_now);

	/* Update the channels now */
	update_task_channels(emu, tr, prev, next);
}

static void
create_task(struct ovni_emu *emu)
{
	if (ovni_payload_size(emu->cur_ev) != 8)
		edie(emu, "cannot create task: unexpected payload size\n");

	uint32_t task_id = emu->cur_ev->payload.u32[0];
	uint32_t type_id = emu->cur_ev->payload.u32[1];

	struct task_info *info = &emu->cur_proc->nosv_task_info;

	task_create(emu, info, type_id, task_id);
}

static void
pre_task(struct ovni_emu *emu)
{
	switch (emu->cur_ev->header.value) {
		case 'c':
			create_task(emu);
			break;
		case 'x':
		case 'e':
		case 'r':
		case 'p': /* Wet floor */
			update_task(emu);
			break;
		default:
			edie(emu, "unexpected task event value %c\n",
				emu->cur_ev->header.value);
	}
}

static void
pre_type(struct ovni_emu *emu)
{
	if (emu->cur_ev->header.value != 'c')
		edie(emu, "unexpected event value %c\n",
			emu->cur_ev->header.value);

	if ((emu->cur_ev->header.flags & OVNI_EV_JUMBO) == 0)
		edie(emu, "expecting a jumbo event\n");

	uint8_t *data = &emu->cur_ev->payload.jumbo.data[0];
	uint32_t typeid = *(uint32_t *) data;
	data += 4;

	const char *label = (const char *) data;

	struct ovni_eproc *proc = emu->cur_proc;

	task_type_create(&proc->nosv_task_info, typeid, label);
}

static void
pre_sched(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch (emu->cur_ev->header.value) {
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

	switch (emu->cur_ev->header.value) {
		case 's':
			chan_push(chan_th, ST_NOSV_API_SUBMIT);
			break;
		case 'S':
			chan_pop(chan_th, ST_NOSV_API_SUBMIT);
			break;
		case 'p':
			chan_push(chan_th, ST_NOSV_API_PAUSE);
			break;
		case 'P':
			chan_pop(chan_th, ST_NOSV_API_PAUSE);
			break;
		case 'y':
			chan_push(chan_th, ST_NOSV_API_YIELD);
			break;
		case 'Y':
			chan_pop(chan_th, ST_NOSV_API_YIELD);
			break;
		case 'w':
			chan_push(chan_th, ST_NOSV_API_WAITFOR);
			break;
		case 'W':
			chan_pop(chan_th, ST_NOSV_API_WAITFOR);
			break;
		case 'c':
			chan_push(chan_th, ST_NOSV_API_SCHEDPOINT);
			break;
		case 'C':
			chan_pop(chan_th, ST_NOSV_API_SCHEDPOINT);
			break;
		default:
			break;
	}
}

static void
pre_mem(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch (emu->cur_ev->header.value) {
		case 'a':
			chan_push(chan_th, ST_NOSV_MEM_ALLOCATING);
			break;
		case 'A':
			chan_pop(chan_th, ST_NOSV_MEM_ALLOCATING);
			break;
		case 'f':
			chan_push(chan_th, ST_NOSV_MEM_FREEING);
			break;
		case 'F':
			chan_pop(chan_th, ST_NOSV_MEM_FREEING);
			break;
		default:
			break;
	}
}

static void
pre_thread_type(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch (emu->cur_ev->header.value) {
		case 'a':
			chan_push(chan_th, ST_NOSV_ATTACH);
			break;
		case 'A':
			chan_pop(chan_th, ST_NOSV_ATTACH);
			break;
		case 'w':
			chan_push(chan_th, ST_NOSV_WORKER);
			break;
		case 'W':
			chan_pop(chan_th, ST_NOSV_WORKER);
			break;
		case 'd':
			chan_push(chan_th, ST_NOSV_DELEGATE);
			break;
		case 'D':
			chan_pop(chan_th, ST_NOSV_DELEGATE);
			break;
		default:
			break;
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

	switch (emu->cur_ev->header.value) {
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

	if (!cpu || cpu->virtual)
		return;

	if (th->state != TH_ST_RUNNING)
		return;

	if (cpu->nrunning_threads > 1) {
		edie(emu, "cpu %s has more than one thread running\n",
			cpu->name);
	}
}

void
hook_pre_nosv(struct ovni_emu *emu)
{
	if (emu->cur_ev->header.model != 'V')
		edie(emu, "hook_pre_nosv: unexpected event with model %c\n",
			emu->cur_ev->header.model);

	if (!emu->cur_thread->is_active)
		edie(emu, "hook_pre_nosv: current thread %d not active\n",
			emu->cur_thread->tid);

	switch (emu->cur_ev->header.category) {
		case 'T':
			pre_task(emu);
			break;
		case 'Y':
			pre_type(emu);
			break;
		case 'S':
			pre_sched(emu);
			break;
		case 'U':
			pre_ss(emu, ST_NOSV_SCHED_SUBMITTING);
			break;
		case 'M':
			pre_mem(emu);
			break;
		case 'H':
			pre_thread_type(emu);
			break;
		case 'A':
			pre_api(emu);
			break;
		default:
			break;
	}

	if (emu->enable_linter)
		check_affinity(emu);
}

void
hook_end_nosv(struct ovni_emu *emu)
{
	/* Emit types for all channel types and processes */
	for (enum chan_type ct = 0; ct < CHAN_MAXTYPE; ct++) {
		struct pcf_file *pcf = &emu->pcf[ct];
		int typeid = chan_to_prvtype[CHAN_NOSV_TYPE];
		struct pcf_type *pcftype = pcf_find_type(pcf, typeid);

		for (size_t i = 0; i < emu->trace.nlooms; i++) {
			struct ovni_loom *loom = &emu->trace.loom[i];
			for (size_t j = 0; j < loom->nprocs; j++) {
				struct ovni_eproc *proc = &loom->proc[j];
				task_create_pcf_types(pcftype, proc->nosv_task_info.types);
			}
		}
	}
}
