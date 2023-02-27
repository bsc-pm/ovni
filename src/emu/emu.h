/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_H
#define EMU_H

#include "bay.h"
#include "trace.h"
#include "emu_args.h"
#include "system.h"
#include "player.h"
#include "model.h"
#include "emu_ev.h"
#include "recorder.h"
#include "emu_stat.h"

struct emu {
	struct bay bay;

	struct emu_args args;
	struct trace trace;
	struct system system;
	struct player player;
	struct model model;
	struct recorder recorder;
	struct emu_stat stat;

	int finished;

	/* Quick access */
	struct stream *stream;
	struct emu_ev *ev;
	struct thread *thread;
	struct proc *proc;
	struct loom *loom;
};

USE_RET int emu_init(struct emu *emu, int argc, char *argv[]);
USE_RET int emu_connect(struct emu *emu);
USE_RET int emu_step(struct emu *emu);
USE_RET int emu_finish(struct emu *emu);

#endif /* EMU_H */
