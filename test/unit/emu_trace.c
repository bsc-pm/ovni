#include "emu/emu_trace.h"
#include "common.h"

int main(void)
{
	char *tracedir = "/home/ram/bsc/ovni/traces/test/ovni";

	struct emu_trace trace;

	if (emu_trace_load(&trace, tracedir) != 0)
		die("emu_trace_load failed\n");

	return 0;
}
