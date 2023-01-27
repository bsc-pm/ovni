/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_H
#define EMU_H

struct emu;

#include "bay.h"
#include "pvtrace.h"
#include "emu_trace.h"
#include "emu_args.h"
#include "system.h"
#include "emu_player.h"
#include "emu_model.h"
#include "emu_ev.h"

enum error_values {
	ST_BAD = 666,
	ST_TOO_MANY_TH = 777,
};

struct emu {
	struct bay bay;
	struct pvman *pvman;

	struct emu_args args;
	struct emu_trace trace;
	struct system system;
	struct emu_player player;
	struct emu_model model;

	/* Quick access */
	struct emu_stream *stream;
	struct emu_ev *ev;
	struct thread *thread;
	struct proc *proc;
	struct loom *loom;
};

int emu_init(struct emu *emu, int argc, char *argv[]);
int emu_step(struct emu *emu);

static inline struct emu *
emu_get(void *ptr)
{
	return (struct emu *) ptr;
}

#endif /* EMU_H */
