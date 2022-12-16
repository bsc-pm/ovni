#include "emu.h"

#include <stdlib.h>

int
main(int argc, char *argv[])
{
	struct ovni_emu *emu = malloc(sizeof(struct ovni_emu));

	if (emu == NULL) {
		perror("malloc");
		return 1;
	}

	emu_init(emu, argc, argv);
	err("emulation starts\n");
	emu_run(emu);
	emu_post(emu);
	emu_destroy(emu);
	err("emulation ends\n");
	free(emu);

	return 0;
}
