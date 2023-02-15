/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef NODES_PRIV_H
#define NODES_PRIV_H

#include "emu.h"
#include "model_cpu.h"
#include "model_thread.h"

/* Private enums */

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
	struct model_thread m;
};

struct nodes_cpu {
	struct model_cpu m;
};

int nodes_probe(struct emu *emu);
int nodes_create(struct emu *emu);
int nodes_connect(struct emu *emu);
int nodes_event(struct emu *emu);
int nodes_finish(struct emu *emu);

#endif /* NODES_PRIV_H */
