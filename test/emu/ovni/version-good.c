/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "instr_ovni.h"

int
main(void)
{
	ovni_version_check();

	/* Create a dummy trace */
	instr_start(0, 1);
	instr_end();

	return 0;
}
