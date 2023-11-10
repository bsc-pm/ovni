/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <ovni.h>
#include "instr.h"

/* Test missing a call to ovni_thread_require() function */

int
main(void)
{
	ovni_version_check();
	ovni_proc_init(1, "node.1", 123);
	ovni_thread_init(123);

	ovni_add_cpu(0, 0);

	/* No ovni_thread_require() call */

	instr_thread_execute(0, -1, 0);

	instr_end();

	return 0;
}
