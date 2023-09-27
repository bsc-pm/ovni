/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
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
	['A'] = {
		['['] = { CH_SUBSYSTEM, PUSH, ST_ATTACHED },
		[']'] = { CH_SUBSYSTEM, POP, ST_ATTACHED },
	},
	['B'] = {
		['j'] = { CH_SUBSYSTEM, PUSH, ST_JOIN_BARRIER },
		['J'] = { CH_SUBSYSTEM, POP, ST_JOIN_BARRIER },
		['b'] = { CH_SUBSYSTEM, PUSH, ST_BARRIER },
		['B'] = { CH_SUBSYSTEM, POP, ST_BARRIER },
		['t'] = { CH_SUBSYSTEM, PUSH, ST_TASKING_BARRIER },
		['T'] = { CH_SUBSYSTEM, POP, ST_TASKING_BARRIER },
		['s'] = { CH_SUBSYSTEM, PUSH, ST_SPIN_WAIT },
		['S'] = { CH_SUBSYSTEM, POP, ST_SPIN_WAIT },
	},
	['W'] = {
		['s'] = { CH_SUBSYSTEM, PUSH, ST_FOR_STATIC },
		['S'] = { CH_SUBSYSTEM, POP, ST_FOR_STATIC },
		['d'] = { CH_SUBSYSTEM, PUSH, ST_FOR_DYNAMIC_INIT },
		['D'] = { CH_SUBSYSTEM, POP, ST_FOR_DYNAMIC_INIT },
		['c'] = { CH_SUBSYSTEM, PUSH, ST_FOR_DYNAMIC_CHUNK },
		['C'] = { CH_SUBSYSTEM, POP, ST_FOR_DYNAMIC_CHUNK },
		['i'] = { CH_SUBSYSTEM, PUSH, ST_SINGLE },
		['I'] = { CH_SUBSYSTEM, POP, ST_SINGLE },
	},
	['T'] = {
		['r'] = { CH_SUBSYSTEM, PUSH, ST_RELEASE_DEPS },
		['R'] = { CH_SUBSYSTEM, POP, ST_RELEASE_DEPS },
		['w'] = { CH_SUBSYSTEM, PUSH, ST_TASKWAIT_DEPS },
		['W'] = { CH_SUBSYSTEM, POP, ST_TASKWAIT_DEPS },
		['['] = { CH_SUBSYSTEM, PUSH, ST_INVOKE_TASK },
		[']'] = { CH_SUBSYSTEM, POP, ST_INVOKE_TASK },
		['i'] = { CH_SUBSYSTEM, PUSH, ST_INVOKE_TASK_IF0 },
		['I'] = { CH_SUBSYSTEM, POP, ST_INVOKE_TASK_IF0 },
		['a'] = { CH_SUBSYSTEM, PUSH, ST_TASK_ALLOC },
		['A'] = { CH_SUBSYSTEM, POP, ST_TASK_ALLOC },
		['s'] = { CH_SUBSYSTEM, PUSH, ST_TASK_SCHEDULE },
		['S'] = { CH_SUBSYSTEM, POP, ST_TASK_SCHEDULE },
		['t'] = { CH_SUBSYSTEM, PUSH, ST_TASKWAIT },
		['T'] = { CH_SUBSYSTEM, POP, ST_TASKWAIT },
		['y'] = { CH_SUBSYSTEM, PUSH, ST_TASKYIELD },
		['Y'] = { CH_SUBSYSTEM, POP, ST_TASKYIELD },
		['d'] = { CH_SUBSYSTEM, PUSH, ST_TASK_DUP_ALLOC },
		['D'] = { CH_SUBSYSTEM, POP, ST_TASK_DUP_ALLOC },
		['c'] = { CH_SUBSYSTEM, PUSH, ST_CHECK_DEPS },
		['C'] = { CH_SUBSYSTEM, POP, ST_CHECK_DEPS },
		['g'] = { CH_SUBSYSTEM, PUSH, ST_TASKGROUP },
		['G'] = { CH_SUBSYSTEM, POP, ST_TASKGROUP },
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
