/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "emu_prv.h"
#include "instr.h"
#include "instr_nanos6.h"
#include "nanos6/nanos6_priv.h"

int
main(void)
{
	instr_start(0, 1);
	instr_nanos6_init();

	instr_nanos6_sponge_enter();

	/* Match the PRV line in the trace */
	FILE *f = fopen("match.sh", "w");
	if (f == NULL)
		die("fopen failed:");

	int type = PRV_NANOS6_SUBSYSTEM;
	int64_t t = get_delta();
	int value = ST_SPONGE;
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/thread.prv\n", t, type, value);
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/cpu.prv\n", t, type, value);

	instr_nanos6_sponge_exit();

	t = get_delta();
	value = 0;
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/thread.prv\n", t, type, value);
	fprintf(f, "grep ':%" PRIi64 ":%d:%d$' ovni/cpu.prv\n", t, type, value);

	fclose(f);

	instr_end();

	return 0;
}
