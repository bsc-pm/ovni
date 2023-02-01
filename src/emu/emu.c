/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 2

//#define ENABLE_DEBUG

#include "emu.h"

#include <unistd.h>
#include "model_ust.h"
#include "models.h"

int
emu_init(struct emu *emu, int argc, char *argv[])
{
	memset(emu, 0, sizeof(*emu));

	emu_args_init(&emu->args, argc, argv);

	/* Load the streams into the trace */
	if (emu_trace_load(&emu->trace, emu->args.tracedir) != 0) {
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

	if (emu_player_init(&emu->player, &emu->trace) != 0) {
		err("emu_init: cannot init player for trace '%s'\n",
				emu->args.tracedir);
		return -1;
	}

//	/* Register all the models */
//	emu_model_register(&emu->model, &ovni_model_spec, emu);

	if (model_ust.create && model_ust.create(emu) != 0) {
		err("model ust create failed");
		return -1;
	}
	if (model_nanos6.create && model_nanos6.create(emu) != 0) {
		err("model nanos6 create failed");
		return -1;
	}


	return 0;
}

int
emu_connect(struct emu *emu)
{
	if (model_ust.connect && model_ust.connect(emu) != 0) {
		err("model ust connect failed");
		return -1;
	}
	if (model_nanos6.connect && model_nanos6.connect(emu) != 0) {
		err("model nanos6 connect failed");
		return -1;
	}
	return 0;
}

static void
set_current(struct emu *emu)
{
	emu->ev = emu_player_ev(&emu->player);
	emu->stream = emu_player_stream(&emu->player);
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
	int ret = emu_player_step(&emu->player);

	/* No more events */
	if (ret > 0)
		return +1;

	/* Error happened */
	if (ret < 0) {
		err("emu_player_step failed");
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
	if (emu->ev->m == 'O' && model_ust.event(emu) != 0) {
		err("ovni event failed");
		panic(emu);
		return -1;
	}

	if (emu->ev->m == '6' && model_nanos6.event(emu) != 0) {
		err("nanos6 event failed");
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
