/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "kernel_priv.h"

static int
context_switch(struct emu *emu)
{
	struct kernel_thread *th = EXT(emu->thread, 'K');
	struct chan *ch = &th->m.ch[CH_CS];

	switch (emu->ev->v) {
		case 'O':
			return chan_push(ch, value_int64(ST_CSOUT));
		case 'I':
			return chan_pop(ch, value_int64(ST_CSOUT));
		default:
			err("unknown context switch event");
			return -1;
	}

	/* Not reached */
	return -1;
}

static int
process_ev(struct emu *emu)
{
	switch (emu->ev->c) {
		case 'C':
			return context_switch(emu);
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
