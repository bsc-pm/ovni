/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 2

#define ENABLE_DEBUG

#include "emu.h"

#include <unistd.h>
#include "models.h"

int
emu_init(struct emu *emu, int argc, char *argv[])
{
	memset(emu, 0, sizeof(*emu));

	emu_args_init(&emu->args, argc, argv);

	/* Load the streams into the trace */
	if (trace_load(&emu->trace, emu->args.tracedir) != 0) {
		err("cannot load trace '%s'\n", emu->args.tracedir);
		return -1;
	}

	/* Parse the trace and build the emu_system */
	if (system_init(&emu->system, &emu->args, &emu->trace) != 0) {
		err("cannot init system for trace '%s'\n",
				emu->args.tracedir);
		return -1;
	}

	/* Place output inside the same tracedir directory */
	if (recorder_init(&emu->recorder, emu->args.tracedir) != 0) {
		err("recorder_init failed");
		return -1;
	}

	/* Initialize the bay */
	bay_init(&emu->bay);

	/* Connect system channels to bay */
	if (system_connect(&emu->system, &emu->bay, &emu->recorder) != 0) {
		err("system_connect failed");
		return -1;
	}

	if (player_init(&emu->player, &emu->trace) != 0) {
		err("cannot init player for trace '%s'\n",
				emu->args.tracedir);
		return -1;
	}

	model_init(&emu->model);

	/* Register all the models */
	if (models_register(&emu->model) != 0) {
		err("failed to register models");
		return -1;
	}

	if (model_probe(&emu->model, emu) != 0) {
		err("model_probe failed");
		return -1;
	}

	if (model_create(&emu->model, emu) != 0) {
		err("model_create failed");
		return -1;
	}

	return 0;
}

int
emu_connect(struct emu *emu)
{
	if (model_connect(&emu->model, emu) != 0) {
		err("model_connect failed");
		return -1;
	}

	return 0;
}

static void
set_current(struct emu *emu)
{
	emu->ev = player_ev(&emu->player);
	emu->stream = player_stream(&emu->player);
	struct lpt *lpt = system_get_lpt(emu->stream);
	emu->loom = lpt->loom;
	emu->proc = lpt->proc;
	emu->thread = lpt->thread;
}

static void
panic(struct emu *emu)
{
	err("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
	err("@@@@@@@@@@@@@@ EMULATOR PANIC @@@@@@@@@@@@@");
	err("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
	if (emu->ev != NULL) {
		err("event: ");
		err("  mcv=%s", emu->ev->mcv);
		err("  rclock=%ld", emu->ev->rclock);
		err("  sclock=%ld", emu->ev->sclock);
		err("  dclock=%ld", emu->ev->dclock);
		err("  payload_size=%ld", emu->ev->payload_size);
		err("  is_jumbo=%d", emu->ev->is_jumbo);
	}

	if (emu->stream != NULL) {
		err("stream: ");
		err("  relpath=%s", emu->stream->relpath);
		err("  offset=%ld", emu->stream->offset);
		err("  clock_offset=%ld", emu->stream->clock_offset);
	}
	err("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
}

int
emu_step(struct emu *emu)
{
	int ret = player_step(&emu->player);

	/* No more events */
	if (ret > 0)
		return +1;

	/* Error happened */
	if (ret < 0) {
		err("player_step failed");
		return -1;
	}

	set_current(emu);

	dbg("----- mvc=%s dclock=%ld -----", emu->ev->mcv, emu->ev->dclock);

	/* Advance recorder clock */
	if (recorder_advance(&emu->recorder, emu->ev->dclock) != 0) {
		err("recorder_advance failed");
		return -1;
	}

	/* Otherwise progress */
	if (model_event(&emu->model, emu, emu->ev->m) != 0) {
		err("model_event failed");
		panic(emu);
		return -1;
	}

	if (bay_propagate(&emu->bay) != 0) {
		err("bay_propagate failed");
		panic(emu);
		return -1;
	}

	return 0;
}

int
emu_finish(struct emu *emu)
{
	if (model_finish(&emu->model, emu) != 0) {
		err("model_finish failed");
		return -1;
	}

	if (recorder_finish(&emu->recorder) != 0) {
		err("recorder_finish failed");
		return -1;
	}

	return 0;
}
