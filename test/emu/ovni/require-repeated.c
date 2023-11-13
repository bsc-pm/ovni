/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <ovni.h>
#include "instr.h"

/* Test multiple calls to ovni_thread_require() function */

int
main(void)
{
	instr_start(0, 1);

	instr_require("ovni");
	instr_require("ovni");

	instr_end();

	return 0;
}
