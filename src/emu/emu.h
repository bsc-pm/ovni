/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_H
#define EMU_H

#include "bay.h"
#include "pvtrace.h"
#include "emu_trace.h"
#include "emu_args.h"
#include "emu_system.h"
#include "emu_player.h"

enum error_values {
	ST_BAD = 666,
	ST_TOO_MANY_TH = 777,
};

struct emu;

typedef int (emu_hook_t)(struct emu *emu);

struct model_spec {
	char *name;
	int model;
	char *depends;
	emu_hook_t *probe;
	emu_hook_t *create;
	emu_hook_t *connect;
	emu_hook_t *event;
};

struct emu {
	struct bay *bay;
	struct pvman *pvman;

	struct emu_args args;
	struct emu_trace trace;
	struct emu_system system;
	struct emu_player player;

	struct model_spec *model[256];
	void *model_ctx[256];
};

int emu_init(struct emu *emu, int argc, char *argv[]);
int emu_step(struct emu *emu);
int emu_model_register(struct emu *emu, struct model_spec *spec, void *ctx);
void *emu_model_get_context(struct emu *emu, struct model_spec *spec, int model);

#endif /* EMU_H */
