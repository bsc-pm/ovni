/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu.h"

#include <stdlib.h>
#include <signal.h>
#include "common.h"

static volatile int run = 1;

static void stop_emulation(int dummy)
{
	UNUSED(dummy);
	run = 0;
	signal(SIGINT, SIG_DFL);
}

int
main(int argc, char *argv[])
{
	progname_set("ovniemu");

	struct emu *emu = calloc(1, sizeof(struct emu));

	if (emu == NULL) {
		err("malloc failed:");
		return 1;
	}

	if (emu_init(emu, argc, argv) != 0) {
		err("emu_init failed");
		return 1;
	}

	if (emu_connect(emu) != 0) {
		err("emu_connect failed");
		return 1;
	}

	signal(SIGINT, stop_emulation);

	info("emulation starts");
	int ret = 0;
	int partial = 0;
	while (run && (ret = emu_step(emu)) == 0);

	if (ret < 0) {
		err("emu_step failed");
		ret = 1;
		/* continue to close the trace files */
		info("emulation aborts!");
	} else {
		ret = 0;
	}

	if (run == 0) {
		info("stopping emulation by user (^C again to abort)");
		partial = 1;
	}

	if (emu_finish(emu) != 0) {
		err("emu_finish failed");
		ret = 1;
	}

	if (ret == 0) {
		if (partial) {
			info("emulation finished partially but ok");
		} else {
			info("emulation finished ok");
		}
	} else {
		info("emulation finished with errors");
	}

	free(emu);

	return ret;
}
