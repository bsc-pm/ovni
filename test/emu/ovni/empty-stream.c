/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <ovni.h>
#include "instr.h"

/* Create a trace where a stream doesn't contain any event */

int
main(void)
{
	instr_start(0, 1);

	/* Don't flush the thread to create an empty stream */

	ovni_proc_fini();

	return 0;
}
