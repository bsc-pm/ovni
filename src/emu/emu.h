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

enum error_values {
	ST_BAD = 666,
	ST_TOO_MANY_TH = 777,
};

struct emu {
	struct bay bay;

	struct emu_args args;
	struct trace trace;
	struct system system;
	struct player player;
	struct model model;
	struct recorder recorder;
	struct emu_stat stat;

	/* Quick access */
	struct stream *stream;
	struct emu_ev *ev;
	struct thread *thread;
	struct proc *proc;
	struct loom *loom;
};

int emu_init(struct emu *emu, int argc, char *argv[]);
int emu_connect(struct emu *emu);
int emu_step(struct emu *emu);
int emu_finish(struct emu *emu);

static inline struct emu *
emu_get(void *ptr)
{
	return (struct emu *) ptr;
}

#endif /* EMU_H */
