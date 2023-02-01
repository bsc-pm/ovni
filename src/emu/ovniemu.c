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

	err("emulation starts\n");
	int ret = 0;
	while ((ret = emu_step(emu)) == 0);

	if (ret < 0)
		die("emu_step failed\n");

	err("emulation ends\n");

	free(emu);

	return 0;
}
