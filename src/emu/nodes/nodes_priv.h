/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef NODES_PRIV_H
#define NODES_PRIV_H

#include "emu.h"
#include "chan.h"
#include "mux.h"
#include "task.h"

/* Private enums */

enum nodes_chan_type {
	CT_TH = 0,
	CT_CPU,
	CT_MAX
};

enum nodes_chan {
	CH_SUBSYSTEM = 0,
	CH_MAX,
};

enum nodes_ss_values {
	ST_REGISTER = 1,
	ST_UNREGISTER,
	ST_IF0_WAIT,
	ST_IF0_INLINE,
	ST_TASKWAIT,
	ST_CREATE,
	ST_SUBMIT,
	ST_SPAWN,
};

struct nodes_thread {
	struct chan *ch;
	struct track *track;
};

struct nodes_cpu {
	struct track *track;
};

int nodes_probe(struct emu *emu);
int nodes_create(struct emu *emu);
int nodes_connect(struct emu *emu);
int nodes_event(struct emu *emu);
int nodes_finish(struct emu *emu);

int nodes_init_pvt(struct emu *emu);
int nodes_finish_pvt(struct emu *emu);
const char *nodes_ss_name(int ss);
int nodes_get_track(int c, int type);

#endif /* NODES_PRIV_H */
