#include "emu.h"

#include <stdlib.h>
#include "common.h"

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

	err("emulation starts");
	int ret = 0;
	while ((ret = emu_step(emu)) == 0);

	if (ret < 0) {
		err("emu_step failed");
		ret = 1;
		/* continue to close the trace files */
		err("emulation aborts!");
	} else {
		err("emulation ends");
		ret = 0;
	}

	if (emu_finish(emu) != 0) {
		err("emu_finish failed");
		ret = 1;
	}

	free(emu);

	return ret;
}
