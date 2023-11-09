/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <ovni.h>
#include "instr.h"

/* Test the ovni_thread_require() function */

int
main(void)
{
	instr_start(0, 1);

	ovni_thread_require("ovni", "1.2.0");

	instr_end();

	return 0;
}
