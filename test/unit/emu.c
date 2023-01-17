#include "emu/emu.h"
#include "common.h"

int main(void)
{
	char *argv[] = {
		"ovniemu",
		"/home/ram/bsc/ovni/traces/test/ovni",
		NULL
	};

	int argc = 2;

	struct emu emu;

	if (emu_init(&emu, argc, argv) != 0)
		die("emu_init failed\n");

	return 0;
}
