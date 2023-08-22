/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include "compat.h"
#include "instr.h"
#include "instr_tampi.h"

int
main(void)
{
	/* Test that a thread ending while the subsystem still has a value in
	 * the stack causes the emulator to fail */

	instr_start(0, 1);

	instr_tampi_library_interface_enter();
	/* The thread is left in the library interface state (should fail) */

	instr_end();

	return 0;
}
