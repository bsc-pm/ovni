/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu.h"
#include <string.h>
#include "emu_ev.h"
#include "models.h"
#include "stream.h"

int
emu_init(struct emu *emu, int argc, char *argv[])
{
	memset(emu, 0, sizeof(*emu));

	emu_args_init(&emu->args, argc, argv);

	/* Load the streams into the trace */
	if (trace_load(&emu->trace, emu->args.tracedir) != 0) {
		err("cannot load trace '%s'", emu->args.tracedir);
		return -1;
	}

	/* Parse the trace and build the emu_system */
	if (system_init(&emu->system, &emu->args, &emu->trace) != 0) {
		err("cannot init system for trace '%s'",
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

	if (player_init(&emu->player, &emu->trace, 0) != 0) {
		err("cannot init player for trace '%s'",
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

	emu_stat_init(&emu->stat);

	return 0;
}

int
emu_connect(struct emu *emu)
{
	if (model_connect(&emu->model, emu) != 0) {
		err("model_connect failed");
		return -1;
	}

	/* Run a propagation phase so we clean all the dirty channels, in
	 * particular the select channel of the muxes */
	if (bay_propagate(&emu->bay) != 0) {
		err("bay_propagate failed");
		return -1;
	}

	return 0;
}

static int
set_current(struct emu *emu)
{
	emu->ev = player_ev(&emu->player);
	emu->stream = player_stream(&emu->player);
	struct lpt *lpt = system_get_lpt(emu->stream);
	if (lpt == NULL) {
		/* For now die if we have a unknown stream */
		err("event from unknown stream: %s",
				emu->stream->path);
		return -1;
	}

	emu->loom = lpt->loom;
	emu->proc = lpt->proc;
	emu->thread = lpt->thread;

	return 0;
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
		err("  rclock=%"PRIi64, emu->ev->rclock);
		err("  sclock=%"PRIi64, emu->ev->sclock);
		err("  dclock=%"PRIi64, emu->ev->dclock);
		err("  payload_size=%zd", emu->ev->payload_size);
		err("  is_jumbo=%d", emu->ev->is_jumbo);
	}

	if (emu->stream != NULL) {
		err("stream: ");
		err("  relpath=%s", emu->stream->relpath);
		err("  offset=%"PRIi64, emu->stream->offset);
		err("  clock_offset=%"PRIi64, emu->stream->clock_offset);
	}
	err("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
}

int
emu_step(struct emu *emu)
{
	int ret = player_step(&emu->player);

	/* No more events */
	if (ret > 0) {
		emu->finished = 1;
		return +1;
	}

	/* Error happened */
	if (ret < 0) {
		err("player_step failed");
		return -1;
	}

	if (set_current(emu) != 0) {
		err("cannot set current event information");
		return -1;
	}

	dbg("----- mcv=%s dclock=%"PRIi64" -----", emu->ev->mcv, emu->ev->dclock);

	emu_stat_update(&emu->stat, &emu->player);

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
	emu_stat_report(&emu->stat, &emu->player, 1);

	int ret = 0;
	if (model_finish(&emu->model, emu) != 0) {
		err("model_finish failed");
		ret = -1;
	}

	/* Finish the traces event if the model_finish failed */
	if (recorder_finish(&emu->recorder) != 0) {
		err("recorder_finish failed");
		ret = -1;
	}

	return ret;
}
