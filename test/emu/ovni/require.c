/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <ovni.h>
#include "instr.h"

/* Test the ovni_thread_require() function */

int
main(void)
{
	instr_start(0, 1);

	/* Override */
	ovni_thread_require("ovni", "666.66.6");

	instr_end();

	return 0;
}
