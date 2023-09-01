/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include "compat.h"
#include "instr.h"
#include "instr_mpi.h"

int
main(void)
{
	/* Test that a thread ending while there is still a mpi function in the
	 * stack causes the emulator to fail */

	instr_start(0, 1);

	instr_mpi_init_thread_enter();
	/* The thread is left in the MPI_Init_thread state (should fail) */

	instr_end();

	return 0;
}
