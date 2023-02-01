#include "emu/trace.h"
#include "common.h"

int main(void)
{
	char *tracedir = "/home/ram/bsc/ovni/traces/test/ovni";

	struct trace trace;

	if (trace_load(&trace, tracedir) != 0)
		die("trace_load failed\n");

	return 0;
}
