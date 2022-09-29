/* Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "uthash.h"
#include "utlist.h"

#include "chan.h"
#include "emu.h"
#include "emu_task.h"
#include "ovni.h"
#include "prv.h"

void
hook_init_nanos6(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_cpu *cpu;
	struct ovni_chan **uth, **ucpu;
	int row;
	FILE *prv_th, *prv_cpu;
	int64_t *clock;

	clock = &emu->delta_time;
	prv_th = emu->prv_thread;
	prv_cpu = emu->prv_cpu;

	/* Init the channels in all threads */
	for (size_t i = 0; i < emu->total_nthreads; i++) {
		th = emu->global_thread[i];
		row = th->gindex + 1;

		uth = &emu->th_chan;

		chan_th_init(th, uth, CHAN_NANOS6_TASKID, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NANOS6_TYPE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NANOS6_SUBSYSTEM, CHAN_TRACK_TH_ACTIVE, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NANOS6_RANK, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_th, clock);
		chan_th_init(th, uth, CHAN_NANOS6_THREAD, CHAN_TRACK_NONE, 0, 1, 1, row, prv_th, clock);
	}

	/* Init the Nanos6 channels in all cpus */
	for (size_t i = 0; i < emu->total_ncpus; i++) {
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;
		ucpu = &emu->cpu_chan;

		chan_cpu_init(cpu, ucpu, CHAN_NANOS6_TASKID, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NANOS6_TYPE, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NANOS6_SUBSYSTEM, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NANOS6_RANK, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
		chan_cpu_init(cpu, ucpu, CHAN_NANOS6_THREAD, CHAN_TRACK_TH_RUNNING, 0, 0, 1, row, prv_cpu, clock);
	}

	/* Init task stack */
	for (size_t i = 0; i < emu->total_nthreads; i++) {
		th = emu->global_thread[i];
		th->nanos6_task_stack.thread = th;
	}
}

/* --------------------------- pre ------------------------------- */

static void
chan_task_stopped(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	th = emu->cur_thread;

	chan_set(&th->chan[CHAN_NANOS6_TASKID], 0);
	chan_set(&th->chan[CHAN_NANOS6_TYPE], 0);

	if (emu->cur_loom->rank_enabled)
		chan_set(&th->chan[CHAN_NANOS6_RANK], 0);
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

	chan_set(&th->chan[CHAN_NANOS6_TASKID], task->id);
	chan_set(&th->chan[CHAN_NANOS6_TYPE], task->type->gid);

	if (emu->cur_loom->rank_enabled)
		chan_set(&th->chan[CHAN_NANOS6_RANK], proc->rank + 1);
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

	/* No need to change the rank as we will switch to tasks from
	 * same thread */
	chan_set(&th->chan[CHAN_NANOS6_TASKID], next->id);

	/* FIXME: We should emit a PRV event even if we are switching to
	 * the same type event, to mark the end of the current task. For
	 * now we only emit a new type if we switch to a type with a
	 * different gid. */
	if (prev->type->gid != next->type->gid)
		chan_set(&th->chan[CHAN_NANOS6_TYPE], next->type->gid);
}

static void
update_task_state(struct ovni_emu *emu)
{
	if (ovni_payload_size(emu->cur_ev) < 4)
		edie(emu, "missing task id in payload\n");

	uint32_t task_id = emu->cur_ev->payload.u32[0];

	struct ovni_ethread *th = emu->cur_thread;
	struct ovni_eproc *proc = emu->cur_proc;

	struct task_info *info = &proc->nanos6_task_info;
	struct task_stack *stack = &th->nanos6_task_stack;

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
			edie(emu, "unexpected Nanos6 task event\n");
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
		case 'r':
			chan_task_running(emu, next);
			break;
		case 'e':
		case 'p':
			chan_task_stopped(emu);
			break;
			/* Additional nested transitions */
		case 'X':
		case 'E':
			chan_task_switch(emu, prev, next);
			break;
		default:
			edie(emu, "unexpected transition value %c\n", tr);
	}
}

static void
enforce_task_rules(struct ovni_emu *emu,
	char tr, struct task *prev, struct task *next)

{
	UNUSED(prev);

	/* If a task has just entered the running state, it must show
	 * the running task body subsystem */
	if (tr == 'x' || tr == 'X') {
		if (next->state != TASK_ST_RUNNING)
			edie(emu, "a Nanos6 task starts running but not in the running state\n");

		struct ovni_ethread *th = emu->cur_thread;
		struct ovni_chan *sschan = &th->chan[CHAN_NANOS6_SUBSYSTEM];
		int st = chan_get_st(sschan);

		if (st != 0 && st != ST_NANOS6_TASK_BODY)
			edie(emu, "a Nanos6 task starts running but not in the \"running body\" subsystem state\n");
	}
}

static void
update_task(struct ovni_emu *emu)
{
	struct ovni_ethread *th = emu->cur_thread;
	struct task_stack *stack = &th->nanos6_task_stack;

	struct task *prev = task_get_running(stack);

	/* Update the emulator state, but don't modify the channels */
	update_task_state(emu);

	struct task *next = task_get_running(stack);

	int was_running = (prev != NULL);
	int runs_now = (next != NULL);
	char tr = expand_transition_value(emu, was_running, runs_now);

	/* Update the task related channels now */
	update_task_channels(emu, tr, prev, next);

	enforce_task_rules(emu, tr, prev, next);
}

static void
create_task(struct ovni_emu *emu)
{
	if (ovni_payload_size(emu->cur_ev) != 8)
		edie(emu, "cannot create task: unexpected payload size\n");

	uint32_t task_id = emu->cur_ev->payload.u32[0];
	uint32_t type_id = emu->cur_ev->payload.u32[1];

	struct task_info *info = &emu->cur_proc->nanos6_task_info;

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
			edie(emu, "unexpected Nanos6 task event value\n");
	}
}

static void
pre_type(struct ovni_emu *emu)
{
	uint8_t value = emu->cur_ev->header.value;

	if (value != 'c')
		edie(emu, "unexpected event value %c\n", value);

	if ((emu->cur_ev->header.flags & OVNI_EV_JUMBO) == 0)
		edie(emu, "expecting a jumbo event\n");

	uint8_t *data = &emu->cur_ev->payload.jumbo.data[0];
	uint32_t typeid = *(uint32_t *) data;
	data += 4;

	const char *label = (const char *) data;

	struct ovni_eproc *proc = emu->cur_proc;

	task_type_create(&proc->nanos6_task_info, typeid, label);
}

static void
pre_deps(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	switch (emu->cur_ev->header.value) {
		case 'r':
			chan_push(chan_th, ST_NANOS6_DEP_REG);
			break;
		case 'R':
			chan_pop(chan_th, ST_NANOS6_DEP_REG);
			break;
		case 'u':
			chan_push(chan_th, ST_NANOS6_DEP_UNREG);
			break;
		case 'U':
			chan_pop(chan_th, ST_NANOS6_DEP_UNREG);
			break;
		default:
			edie(emu, "unknown Nanos6 dependency event\n");
	}
}

static void
pre_blocking(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	switch (emu->cur_ev->header.value) {
		case 'b':
			chan_push(chan_th, ST_NANOS6_BLK_BLOCKING);
			break;
		case 'B':
			chan_pop(chan_th, ST_NANOS6_BLK_BLOCKING);
			break;
		case 'u':
			chan_push(chan_th, ST_NANOS6_BLK_UNBLOCKING);
			break;
		case 'U':
			chan_pop(chan_th, ST_NANOS6_BLK_UNBLOCKING);
			break;
		case 'w':
			chan_push(chan_th, ST_NANOS6_BLK_TASKWAIT);
			break;
		case 'W':
			chan_pop(chan_th, ST_NANOS6_BLK_TASKWAIT);
			break;
		case 'f':
			chan_push(chan_th, ST_NANOS6_BLK_WAITFOR);
			break;
		case 'F':
			chan_pop(chan_th, ST_NANOS6_BLK_WAITFOR);
			break;
		default:
			edie(emu, "unknown Nanos6 blocking event\n");
	}
}

static void
pre_worker(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	switch (emu->cur_ev->header.value) {
		case '[':
			chan_push(chan_th, ST_NANOS6_WORKER_LOOP);
			break;
		case ']':
			chan_pop(chan_th, ST_NANOS6_WORKER_LOOP);
			break;
		case 't':
			chan_push(chan_th, ST_NANOS6_HANDLING_TASK);
			break;
		case 'T':
			chan_pop(chan_th, ST_NANOS6_HANDLING_TASK);
			break;
		case 'w':
			chan_push(chan_th, ST_NANOS6_SWITCH_TO);
			break;
		case 'W':
			chan_pop(chan_th, ST_NANOS6_SWITCH_TO);
			break;
		case 'm':
			chan_push(chan_th, ST_NANOS6_MIGRATE);
			break;
		case 'M':
			chan_pop(chan_th, ST_NANOS6_MIGRATE);
			break;
		case 's':
			chan_push(chan_th, ST_NANOS6_SUSPEND);
			break;
		case 'S':
			chan_pop(chan_th, ST_NANOS6_SUSPEND);
			break;
		case 'r':
			chan_push(chan_th, ST_NANOS6_RESUME);
			break;
		case 'R':
			chan_pop(chan_th, ST_NANOS6_RESUME);
			break;
		case '*':
			chan_ev(chan_th, EV_NANOS6_SIGNAL);
			break;
		default:
			edie(emu, "unknown Nanos6 worker event\n");
	}
}

static void
pre_memory(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	switch (emu->cur_ev->header.value) {
		case 'a':
			chan_push(chan_th, ST_NANOS6_ALLOCATING);
			break;
		case 'A':
			chan_pop(chan_th, ST_NANOS6_ALLOCATING);
			break;
		case 'f':
			chan_push(chan_th, ST_NANOS6_FREEING);
			break;
		case 'F':
			chan_pop(chan_th, ST_NANOS6_FREEING);
			break;
		default:
			edie(emu, "unknown Nanos6 memory event\n");
	}
}

static void
pre_sched(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_SUBSYSTEM];

	switch (emu->cur_ev->header.value) {
		case '[':
			chan_push(chan_th, ST_NANOS6_SCHED_SERVING);
			break;
		case ']':
			chan_pop(chan_th, ST_NANOS6_SCHED_SERVING);
			break;
		case 'a':
			chan_push(chan_th, ST_NANOS6_SCHED_ADDING);
			break;
		case 'A':
			chan_pop(chan_th, ST_NANOS6_SCHED_ADDING);
			break;
		case 'p':
			chan_push(chan_th, ST_NANOS6_SCHED_PROCESSING);
			break;
		case 'P':
			chan_pop(chan_th, ST_NANOS6_SCHED_PROCESSING);
			break;
		case '@':
			chan_ev(chan_th, EV_NANOS6_SCHED_SELF);
			break;
		case 'r':
			chan_ev(chan_th, EV_NANOS6_SCHED_RECV);
			break;
		case 's':
			chan_ev(chan_th, EV_NANOS6_SCHED_SEND);
			break;
		default:
			edie(emu, "unknown Nanos6 scheduler event\n");
	}
}

static void
pre_thread(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NANOS6_THREAD];

	switch (emu->cur_ev->header.value) {
		case 'e':
			chan_push(chan_th, ST_NANOS6_TH_EXTERNAL);
			break;
		case 'E':
			chan_pop(chan_th, ST_NANOS6_TH_EXTERNAL);
			break;
		case 'w':
			chan_push(chan_th, ST_NANOS6_TH_WORKER);
			break;
		case 'W':
			chan_pop(chan_th, ST_NANOS6_TH_WORKER);
			break;
		case 'l':
			chan_push(chan_th, ST_NANOS6_TH_LEADER);
			break;
		case 'L':
			chan_pop(chan_th, ST_NANOS6_TH_LEADER);
			break;
		case 'm':
			chan_push(chan_th, ST_NANOS6_TH_MAIN);
			break;
		case 'M':
			chan_pop(chan_th, ST_NANOS6_TH_MAIN);
			break;
		default:
			edie(emu, "unknown Nanos6 thread type event\n");
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

	switch (emu->cur_ev->header.value) {
		case '[':
			chan_push(chan_th, st);
			break;
		case ']':
			chan_pop(chan_th, st);
			break;
		default:
			edie(emu, "unexpected value '%c' (expecting '[' or ']')\n",
				emu->cur_ev->header.value);
	}
}

static void
check_affinity(struct ovni_emu *emu)
{
	struct ovni_ethread *th = emu->cur_thread;
	struct ovni_cpu *cpu = th->cpu;

	if (!cpu || cpu->virtual)
		return;

	if (cpu->nrunning_threads > 1) {
		eerr(emu, "cpu %s has more than one thread running\n", cpu->name);

		/* Only abort in linter mode so we can still see the
		 * trace to find out what was happening */
		if (emu->enable_linter)
			abort();
	}
}

void
hook_pre_nanos6(struct ovni_emu *emu)
{
	if (emu->cur_ev->header.model != '6')
		edie(emu, "hook_pre_nanos6: unexpected event with model %c\n",
			emu->cur_ev->header.model);

	if (!emu->cur_thread->is_active) {
		edie(emu, "hook_pre_nanos6: current thread %d not active\n",
			emu->cur_thread->tid);
	}

	switch (emu->cur_ev->header.category) {
		case 'T':
			pre_task(emu);
			break;
		case 'C':
			pre_ss(emu, ST_NANOS6_TASK_CREATING);
			break;
		case 'Y':
			pre_type(emu);
			break;
		case 'S':
			pre_sched(emu);
			break;
		case 'U':
			pre_ss(emu, ST_NANOS6_TASK_SUBMIT);
			break;
		case 'F':
			pre_ss(emu, ST_NANOS6_TASK_SPAWNING);
			break;
		case 'O':
			pre_ss(emu, ST_NANOS6_TASK_FOR);
			break;
		case 't':
			pre_ss(emu, ST_NANOS6_TASK_BODY);
			break;
		case 'H':
			pre_thread(emu);
			break;
		case 'D':
			pre_deps(emu);
			break;
		case 'B':
			pre_blocking(emu);
			break;
		case 'W':
			pre_worker(emu);
			break;
		case 'M':
			pre_memory(emu);
			break;
		default:
			edie(emu, "unknown Nanos6 event category\n");
	}

	check_affinity(emu);
}

static void
end_lint(struct ovni_emu *emu)
{
	/* Ensure we run out of subsystem states */
	for (size_t i = 0; i < emu->total_nthreads; i++) {
		struct ovni_ethread *th = emu->global_thread[i];
		struct ovni_chan *ch = &th->chan[CHAN_NANOS6_SUBSYSTEM];
		if (ch->n != 1) {
			int top = ch->stack[ch->n - 1];

			struct pcf_value_label *pv;
			char *name = "(unknown)";
			for (pv = &nanos6_ss_values[0]; pv->label; pv++) {
				if (pv->value == top) {
					name = pv->label;
					break;
				}
			}

			die("thread %d ended with %d extra stacked nanos6 subsystems, top=\"%s\"\n",
				th->tid, ch->n - 1, name);
		}
	}
}

void
hook_end_nanos6(struct ovni_emu *emu)
{
	/* Emit types for all channel types and processes */
	for (enum chan_type ct = 0; ct < CHAN_MAXTYPE; ct++) {
		struct pcf_file *pcf = &emu->pcf[ct];
		int typeid = chan_to_prvtype[CHAN_NANOS6_TYPE];
		struct pcf_type *pcftype = pcf_find_type(pcf, typeid);

		for (size_t i = 0; i < emu->trace.nlooms; i++) {
			struct ovni_loom *loom = &emu->trace.loom[i];
			for (size_t j = 0; j < loom->nprocs; j++) {
				struct ovni_eproc *proc = &loom->proc[j];
				task_create_pcf_types(pcftype, proc->nanos6_task_info.types);
			}
		}
	}

	/* When running in linter mode perform additional checks */
	if (emu->enable_linter)
		end_lint(emu);
}
