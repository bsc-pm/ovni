/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "openmp_priv.h"
#include "chan.h"
#include "common.h"
#include "emu.h"
#include "emu_ev.h"
#include "extend.h"
#include "model_thread.h"
#include "ovni.h"
#include "proc.h"
#include "task.h"
#include "thread.h"
#include "value.h"

enum { PUSH = 1, POP = 2, IGN = 3 };

static const int fn_table[256][256][3] = {
	['B'] = {
		['b'] = { CH_SUBSYSTEM, PUSH, ST_BARRIER_PLAIN },
		['B'] = { CH_SUBSYSTEM, POP,  ST_BARRIER_PLAIN },
		['j'] = { CH_SUBSYSTEM, PUSH, ST_BARRIER_JOIN },
		['J'] = { CH_SUBSYSTEM, POP,  ST_BARRIER_JOIN },
		['f'] = { CH_SUBSYSTEM, PUSH, ST_BARRIER_FORK },
		['F'] = { CH_SUBSYSTEM, POP,  ST_BARRIER_FORK },
		['t'] = { CH_SUBSYSTEM, PUSH, ST_BARRIER_TASK },
		['T'] = { CH_SUBSYSTEM, POP,  ST_BARRIER_TASK },
		['s'] = { CH_SUBSYSTEM, IGN,  ST_BARRIER_SPIN_WAIT },
		['S'] = { CH_SUBSYSTEM, IGN,  ST_BARRIER_SPIN_WAIT },
	},
	['I'] = {
		['a'] = { CH_SUBSYSTEM, PUSH, ST_CRITICAL_ACQ },
		['A'] = { CH_SUBSYSTEM, POP,  ST_CRITICAL_ACQ },
		['r'] = { CH_SUBSYSTEM, PUSH, ST_CRITICAL_REL },
		['R'] = { CH_SUBSYSTEM, POP,  ST_CRITICAL_REL },
		['['] = { CH_SUBSYSTEM, PUSH, ST_CRITICAL_SECTION },
		[']'] = { CH_SUBSYSTEM, POP,  ST_CRITICAL_SECTION },
	},
	['W'] = {
		['d'] = { CH_SUBSYSTEM, PUSH, ST_WD_DISTRIBUTE },
		['D'] = { CH_SUBSYSTEM, POP,  ST_WD_DISTRIBUTE },
		['c'] = { CH_SUBSYSTEM, PUSH, ST_WD_FOR_DYNAMIC_CHUNK },
		['C'] = { CH_SUBSYSTEM, POP,  ST_WD_FOR_DYNAMIC_CHUNK },
		['y'] = { CH_SUBSYSTEM, PUSH, ST_WD_FOR_DYNAMIC_INIT },
		['Y'] = { CH_SUBSYSTEM, POP,  ST_WD_FOR_DYNAMIC_INIT },
		['s'] = { CH_SUBSYSTEM, PUSH, ST_WD_FOR_STATIC },
		['S'] = { CH_SUBSYSTEM, POP,  ST_WD_FOR_STATIC },
		['e'] = { CH_SUBSYSTEM, PUSH, ST_WD_SECTION },
		['E'] = { CH_SUBSYSTEM, POP,  ST_WD_SECTION },
		['i'] = { CH_SUBSYSTEM, PUSH, ST_WD_SINGLE },
		['I'] = { CH_SUBSYSTEM, POP,  ST_WD_SINGLE },
	},
	['T'] = {
		['a'] = { CH_SUBSYSTEM, PUSH, ST_TASK_ALLOC },
		['A'] = { CH_SUBSYSTEM, POP,  ST_TASK_ALLOC },
		['c'] = { CH_SUBSYSTEM, PUSH, ST_TASK_CHECK_DEPS },
		['C'] = { CH_SUBSYSTEM, POP,  ST_TASK_CHECK_DEPS },
		['d'] = { CH_SUBSYSTEM, PUSH, ST_TASK_DUP_ALLOC },
		['D'] = { CH_SUBSYSTEM, POP,  ST_TASK_DUP_ALLOC },
		['r'] = { CH_SUBSYSTEM, PUSH, ST_TASK_RELEASE_DEPS },
		['R'] = { CH_SUBSYSTEM, POP,  ST_TASK_RELEASE_DEPS },
		['['] = { CH_SUBSYSTEM, PUSH, ST_TASK_RUN },
		[']'] = { CH_SUBSYSTEM, POP,  ST_TASK_RUN },
		['i'] = { CH_SUBSYSTEM, PUSH, ST_TASK_RUN_IF0 },
		['I'] = { CH_SUBSYSTEM, POP,  ST_TASK_RUN_IF0 },
		['s'] = { CH_SUBSYSTEM, PUSH, ST_TASK_SCHEDULE },
		['S'] = { CH_SUBSYSTEM, POP,  ST_TASK_SCHEDULE },
		['g'] = { CH_SUBSYSTEM, PUSH, ST_TASK_TASKGROUP },
		['G'] = { CH_SUBSYSTEM, POP,  ST_TASK_TASKGROUP },
		['t'] = { CH_SUBSYSTEM, PUSH, ST_TASK_TASKWAIT },
		['T'] = { CH_SUBSYSTEM, POP,  ST_TASK_TASKWAIT },
		['w'] = { CH_SUBSYSTEM, PUSH, ST_TASK_TASKWAIT_DEPS },
		['W'] = { CH_SUBSYSTEM, POP,  ST_TASK_TASKWAIT_DEPS },
		['y'] = { CH_SUBSYSTEM, PUSH, ST_TASK_TASKYIELD },
		['Y'] = { CH_SUBSYSTEM, POP,  ST_TASK_TASKYIELD },
	},
	['A'] = {
		['['] = { CH_SUBSYSTEM, PUSH, ST_RT_ATTACHED },
		[']'] = { CH_SUBSYSTEM, POP,  ST_RT_ATTACHED },
	},
	['M'] = {
		['i'] = { CH_SUBSYSTEM, PUSH, ST_RT_MICROTASK_INTERNAL },
		['I'] = { CH_SUBSYSTEM, POP,  ST_RT_MICROTASK_INTERNAL },
		['u'] = { CH_SUBSYSTEM, PUSH, ST_RT_MICROTASK_USER },
		['U'] = { CH_SUBSYSTEM, POP,  ST_RT_MICROTASK_USER },
	},
	['H'] = {
		['['] = { CH_SUBSYSTEM, PUSH, ST_RT_WORKER_LOOP },
		[']'] = { CH_SUBSYSTEM, POP,  ST_RT_WORKER_LOOP },
	},
	['C'] = {
		['i'] = { CH_SUBSYSTEM, PUSH, ST_RT_INIT },
		['I'] = { CH_SUBSYSTEM, POP,  ST_RT_INIT },
		['f'] = { CH_SUBSYSTEM, PUSH, ST_RT_FORK_CALL },
		['F'] = { CH_SUBSYSTEM, POP,  ST_RT_FORK_CALL },
	},
};

static int
simple(struct emu *emu)
{
	if (!emu->thread->is_running) {
		err("current thread %d not running", emu->thread->tid);
		return -1;
	}

	const int *entry = fn_table[emu->ev->c][emu->ev->v];
	int chind = entry[0];
	int action = entry[1];
	int st = entry[2];

	struct openmp_thread *th = EXT(emu->thread, 'P');
	struct chan *ch = &th->m.ch[chind];

	if (action == PUSH) {
		return chan_push(ch, value_int64(st));
	} else if (action == POP) {
		return chan_pop(ch, value_int64(st));
	} else if (action == IGN) {
		return 0; /* do nothing */
	}

	err("unknown openmp function event");
	return -1;
}

static int
create_task(struct emu *emu)
{
	if (emu->ev->payload_size != 8) {
		err("unexpected payload size");
		return -1;
	}

	uint32_t taskid = emu->ev->payload->u32[0];
	uint32_t typeid = emu->ev->payload->u32[1];

	if (taskid == 0) {
		err("taskid cannot be 0");
		return -1;
	}

	if (typeid == 0) {
		err("typeid cannot be 0");
		return -1;
	}

	struct openmp_proc *proc = EXT(emu->proc, 'P');
	struct task_info *info = &proc->task_info;

	/* OpenMP submits inline tasks without pausing the previous
	 * task, so we relax the model to allow this for now. */
	uint32_t flags = TASK_FLAG_RELAX_NESTING;

	/* https://gitlab.pm.bsc.es/rarias/ovni/-/issues/208 */
	flags |= TASK_FLAG_RESURRECT;

	if (task_create(info, typeid, taskid, flags) != 0) {
		err("task_create failed");
		return -1;
	}

	dbg("task created with taskid %u", taskid);

	return 0;
}

static int
update_task(struct emu *emu)
{
	if (emu->ev->payload_size < 4) {
		err("missing task id in payload");
		return -1;
	}

	uint32_t taskid = emu->ev->payload->u32[0];

	if (taskid == 0) {
		err("taskid cannot be 0");
		return -1;
	}

	struct openmp_thread *th = EXT(emu->thread, 'P');
	struct openmp_proc *proc = EXT(emu->proc, 'P');

	struct task_info *info = &proc->task_info;
	struct task_stack *stack = &th->task_stack;

	struct task *task = task_find(info->tasks, taskid);

	if (task == NULL) {
		err("cannot find task with id %u", taskid);
		return -1;
	}

	/* OpenMP doesn't have parallel tasks */
	uint32_t body_id = 1;

	if (emu->ev->v == 'x') {
		if (task_execute(stack, task, body_id) != 0) {
			err("cannot change task state to running");
			return -1;
		}
		if (chan_push(&th->m.ch[CH_TASKID], value_int64(task->id)) != 0) {
			err("chan_push taskid failed");
			return -1;
		}
		if (chan_push(&th->m.ch[CH_LABEL], value_int64(task->type->gid)) != 0) {
			err("chan_push task label failed");
			return -1;
		}
	} else if (emu->ev->v == 'e') {
		if (task_end(stack, task, body_id) != 0) {
			err("cannot change task state to end");
			return -1;
		}
		if (chan_pop(&th->m.ch[CH_TASKID], value_int64(task->id)) != 0) {
			err("chan_pop taskid failed");
			return -1;
		}
		if (chan_pop(&th->m.ch[CH_LABEL], value_int64(task->type->gid)) != 0) {
			err("chan_pop task label failed");
			return -1;
		}
	} else {
		err("unexpected task event %c", emu->ev->v);
		return -1;
	}


	return 0;
}

static int
pre_task(struct emu *emu)
{
	int ret = 0;
	switch (emu->ev->v) {
		case 'c':
			ret = create_task(emu);
			break;
		case 'x':
		case 'e':
			ret = update_task(emu);
			break;
		default:
			err("unexpected task event value");
			return -1;
	}

	if (ret != 0) {
		err("cannot update task state");
		return -1;
	}

	return 0;
}

static int
pre_type(struct emu *emu)
{
	uint8_t value = emu->ev->v;

	if (value != 'c') {
		err("unexpected event value %c", value);
		return -1;
	}

	if (!emu->ev->is_jumbo) {
		err("expecting a jumbo event");
		return -1;
	}

	const uint8_t *data = &emu->ev->payload->jumbo.data[0];
	uint32_t typeid;
	memcpy(&typeid, data, 4); /* May be unaligned */
	data += 4;

	const char *label = (const char *) data;

	struct openmp_proc *proc = EXT(emu->proc, 'P');
	struct task_info *info = &proc->task_info;

	/* It will be used for tasks and worksharings. */
	if (task_type_create(info, typeid, label) != 0) {
		err("task_type_create failed");
		return -1;
	}

	return 0;
}

static int
update_ws_state(struct emu *emu, uint8_t action)
{
	if (emu->ev->payload_size < 4) {
		err("missing worksharing id in payload");
		return -1;
	}

	uint32_t typeid = emu->ev->payload->u32[0];

	if (typeid == 0) {
		err("worksharing type id cannot be 0");
		return -1;
	}

	struct openmp_thread *th = EXT(emu->thread, 'P');
	struct openmp_proc *proc = EXT(emu->proc, 'P');

	struct task_info *info = &proc->task_info;

	/* Worksharings share the task type */
	struct task_type *ttype = task_type_find(info->types, typeid);

	if (ttype == NULL) {
		err("cannot find ws with type %"PRIu32, typeid);
		return -1;
	}

	if (action == 'x') {
		if (chan_push(&th->m.ch[CH_LABEL], value_int64(ttype->gid)) != 0) {
			err("chan_push worksharing label failed");
			return -1;
		}
	} else {
		if (chan_pop(&th->m.ch[CH_LABEL], value_int64(ttype->gid)) != 0) {
			err("chan_pop worksharing label failed");
			return -1;
		}
	}

	return 0;
}

static int
pre_worksharing(struct emu *emu)
{
	int ret = 0;
	switch (emu->ev->v) {
		case 'x':
		case 'e':
			ret = update_ws_state(emu, emu->ev->v);
			break;
		default:
			err("unexpected ws event value %c", emu->ev->v);
			return -1;
	}

	if (ret != 0) {
		err("cannot update worksharing channels");
		return -1;
	}

	return 0;
}

static int
process_ev(struct emu *emu)
{
	if (!emu->thread->is_running) {
		err("current thread %d not running", emu->thread->tid);
		return -1;
	}
	switch (emu->ev->c) {
		case 'B':
		case 'I':
		case 'W':
		case 'T':
		case 'A':
		case 'M':
		case 'H':
		case 'C':
			return simple(emu);
		case 'P':
			return pre_task(emu);
		case 'O':
			return pre_type(emu);
		case 'Q':
			return pre_worksharing(emu);
	}

	err("unknown event category");
	return -1;
}

int
model_openmp_event(struct emu *emu)
{
	dbg("in openmp_event");
	if (emu->ev->m != 'P') {
		err("unexpected event model %c", emu->ev->m);
		return -1;
	}

	dbg("got openmp event %s", emu->ev->mcv);
	if (process_ev(emu) != 0) {
		err("error processing openmp event");
		return -1;
	}

	return 0;
}
