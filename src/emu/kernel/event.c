/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "kernel_priv.h"

enum { PUSH = 1, POP = 2, IGN = 3 };

static const int ss_table[256][256][3] = {
	['C'] = {
		['O'] = { CH_CS, PUSH, ST_CSOUT },
		['I'] = { CH_CS, POP,  ST_CSOUT },
	},
};

static int
simple(struct emu *emu)
{
	const int *entry = ss_table[emu->ev->c][emu->ev->v];
	int chind = entry[0];
	int action = entry[1];
	int st = entry[2];

	struct kernel_thread *th = EXT(emu->thread, 'K');
	struct chan *ch = &th->m.ch[chind];

	if (action == PUSH) {
		return chan_push(ch, value_int64(st));
	} else if (action == POP) {
		return chan_pop(ch, value_int64(st));
	} else if (action == IGN) {
		return 0; /* do nothing */
	} else {
		err("unknown event");
		return -1;
	}

	return 0;
}

static int
process_ev(struct emu *emu)
{
	switch (emu->ev->c) {
		case 'C':
			return simple(emu);
		default:
			err("unknown Kernel event category");
			return -1;
	}

	/* Not reached */
	return 0;
}

int
model_kernel_event(struct emu *emu)
{
	static int enabled = 0;

	if (!enabled) {
		if (model_kernel_connect(emu) != 0) {
			err("kernel_connect failed");
			return -1;
		}
		enabled = 1;
	}


	dbg("in kernel_event");
	if (emu->ev->m != 'K') {
		err("unexpected event model %c\n", emu->ev->m);
		return -1;
	}

	dbg("got kernel event %s", emu->ev->mcv);
	if (process_ev(emu) != 0) {
		err("error processing Kernel event");
		return -1;
	}

	return 0;
}
