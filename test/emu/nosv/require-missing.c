/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <ovni.h>
#include "instr_nosv.h"

/* Test the emulator aborts if a stream contains require models but they don't
 * cover all the events (in this case nosv). */

int
main(void)
{
	instr_start(0, 1);

	/* Don't enable nosv model via instr_nosv_init() */

	/* Emit a nosv event */
	instr_nosv_type_create(666);

	instr_end();

	return 0;
}
