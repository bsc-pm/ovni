/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 2

#include "emu.h"

#include <unistd.h>

int
emu_model_register(struct emu *emu, struct model_spec *spec, void *ctx)
{
	emu->model_ctx[spec->model] = ctx;
	emu->model[spec->model] = spec;
	return 0;
}

void *
emu_model_get_context(struct emu *emu, struct model_spec *spec, int model)
{
	for (int i = 0; spec->depends[i]; i++) {
		if (spec->depends[i] == model)
			return emu->model_ctx[model];
	}

	/* Not allowed */
	return NULL;
}

int
emu_init(struct emu *emu, int argc, char *argv[])
{
	memset(emu, 0, sizeof(*emu));

	emu_args_init(&emu->args, argc, argv);

	/* Load the streams into the trace */
	if (emu_trace_load(&emu->trace, emu->args.tracedir) != 0) {
		err("emu_init: cannot load trace '%s'\n",
				emu->args.tracedir);
		return -1;
	}

	/* Parse the trace and build the emu_system */
	if (emu_system_init(&emu->system, &emu->args, &emu->trace) != 0) {
		err("emu_init: cannot init system for trace '%s'\n",
				emu->args.tracedir);
		return -1;
	}

	if (emu_player_init(&emu->player, &emu->trace) != 0) {
		err("emu_init: cannot init player for trace '%s'\n",
				emu->args.tracedir);
		return -1;
	}

	/* Register all the models */
	//emu_model_register(emu, ovni_model_spec, ctx);

	return 0;
}

int
emu_step(struct emu *emu)
{
	int ret = emu_player_step(&emu->player);

	/* No more events */
	if (ret > 0)
		return +1;

	/* Error happened */
	if (ret < 0) {
		err("emu_step: emu_player_step failed\n");
		return -1;
	}

	/* Otherwise progress */
	return 0;
}
