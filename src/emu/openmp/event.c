/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "openmp_priv.h"
#include "chan.h"
#include "common.h"
#include "emu.h"
#include "emu_ev.h"
#include "extend.h"
#include "model_thread.h"
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
process_ev(struct emu *emu)
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
