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

	int type = PRV_NANOS6_BREAKDOWN;
	FILE *f = fopen("match.sh", "w");
	if (f == NULL)
		die("fopen failed:");

	instr_nanos6_worker_loop_enter();

	/* Enter sponge subsystem */
	instr_nanos6_sponge_enter();

	/* Set state to Absorbing */
	instr_nanos6_absorbing();

	/* Ensure the only row in breakdown is in absorbing */
	fprintf(f, "grep '1:%" PRIi64 ":%d:%d$' ovni/nanos6-breakdown.prv\n",
			get_delta(), type, ST_ABSORBING);

	/* Set state to Resting */
	instr_nanos6_resting();

	/* Ensure the only row in breakdown is in Resting */
	fprintf(f, "grep '1:%" PRIi64 ":%d:%d$' ovni/nanos6-breakdown.prv\n",
			get_delta(), type, ST_RESTING);

	instr_nanos6_progressing();

	/* Now the state must follow the subsystem, which should be
	 * sponge mode */
	fprintf(f, "grep '1:%" PRIi64 ":%d:%d$' ovni/nanos6-breakdown.prv\n",
			get_delta(), type, ST_SPONGE);

	fclose(f);

	instr_nanos6_sponge_exit();

	instr_nanos6_worker_loop_exit();

	instr_end();

	return 0;
}
