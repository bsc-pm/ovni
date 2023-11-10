/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "tampi_priv.h"
#include "chan.h"
#include "common.h"
#include "emu.h"
#include "emu_ev.h"
#include "extend.h"
#include "model_thread.h"
#include "thread.h"
#include "value.h"

enum { PUSH = 1, POP = 2, IGN = 3 };

#define CHSS CH_SUBSYSTEM

static const int ss_table[256][256][3] = {
	['C'] = {
		['i'] = { CHSS, PUSH, ST_COMM_ISSUE_NONBLOCKING },
		['I'] = { CHSS, POP,  ST_COMM_ISSUE_NONBLOCKING },
	},
	['G'] = {
		['c'] = { CHSS, PUSH, ST_GLOBAL_ARRAY_CHECK },
		['C'] = { CHSS, POP,  ST_GLOBAL_ARRAY_CHECK },
	},
	['L'] = {
		['i'] = { CHSS, PUSH, ST_LIBRARY_INTERFACE },
		['I'] = { CHSS, POP,  ST_LIBRARY_INTERFACE },
		['p'] = { CHSS, PUSH, ST_LIBRARY_POLLING },
		['P'] = { CHSS, POP,  ST_LIBRARY_POLLING },
	},
	['Q'] = {
		['a'] = { CHSS, PUSH, ST_QUEUE_ADD },
		['A'] = { CHSS, POP,  ST_QUEUE_ADD },
		['t'] = { CHSS, PUSH, ST_QUEUE_TRANSFER },
		['T'] = { CHSS, POP,  ST_QUEUE_TRANSFER },
	},
	['R'] = {
		['c'] = { CHSS, PUSH, ST_REQUEST_COMPLETED },
		['C'] = { CHSS, POP,  ST_REQUEST_COMPLETED },
		['t'] = { CHSS, PUSH, ST_REQUEST_TEST },
		['T'] = { CHSS, POP,  ST_REQUEST_TEST },
		['a'] = { CHSS, PUSH, ST_REQUEST_TESTALL },
		['A'] = { CHSS, POP,  ST_REQUEST_TESTALL },
		['s'] = { CHSS, PUSH, ST_REQUEST_TESTSOME },
		['S'] = { CHSS, POP,  ST_REQUEST_TESTSOME },
	},
	['T'] = {
		['c'] = { CHSS, PUSH, ST_TICKET_CREATE },
		['C'] = { CHSS, POP,  ST_TICKET_CREATE },
		['w'] = { CHSS, PUSH, ST_TICKET_WAIT },
		['W'] = { CHSS, POP,  ST_TICKET_WAIT },
	},
};

static int
process_ev(struct emu *emu)
{
	if (!emu->thread->is_running) {
		err("current thread %d not running", emu->thread->tid);
		return -1;
	}

	const int *entry = ss_table[emu->ev->c][emu->ev->v];
	int chind = entry[0];
	int action = entry[1];
	int st = entry[2];

	struct tampi_thread *th = EXT(emu->thread, 'T');
	struct chan *ch = &th->m.ch[chind];

	if (action == PUSH) {
		return chan_push(ch, value_int64(st));
	} else if (action == POP) {
		return chan_pop(ch, value_int64(st));
	} else if (action == IGN) {
		return 0; /* do nothing */
	}

	err("unknown TAMPI subsystem event");
	return -1;
}

int
model_tampi_event(struct emu *emu)
{
	dbg("in tampi_event");
	if (emu->ev->m != 'T') {
		err("unexpected event model %c", emu->ev->m);
		return -1;
	}

	dbg("got tampi event %s", emu->ev->mcv);
	if (process_ev(emu) != 0) {
		err("error processing TAMPI event");
		return -1;
	}

	return 0;
}
