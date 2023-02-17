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

	if (emu_init(emu, argc, argv) != 0)
		die("emu_init failed\n");

	if (emu_connect(emu) != 0)
		die("emu_connect failed\n");

	signal(SIGINT, stop_emulation);

	err("emulation starts");
	int ret = 0;
	while (run && (ret = emu_step(emu)) == 0);

	if (ret < 0) {
		err("emu_step failed");
		ret = 1;
		/* continue to close the trace files */
		err("emulation aborts!");
	} else {
		ret = 0;
	}

	if (run == 0)
		err("stopping emulation by user (^C again to abort)");

	if (emu_finish(emu) != 0) {
		err("emu_finish failed");
		ret = 1;
	}

	if (ret == 0)
		err("emulation ends ok");
	else
		err("emulation ends with errors");

	free(emu);

	return ret;
}
