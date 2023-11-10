/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include "compat.h"
#include "instr.h"
#include "instr_mpi.h"

int
main(void)
{
	/* Test that a thread calling the mpi function that is already executing
	 * causes the emulator to fail */

	instr_start(0, 1);
	instr_mpi_init();

	instr_mpi_init_thread_enter();
	/* The thread runs the same mpi function in a nested way (should fail) */
	instr_mpi_init_thread_enter();
	instr_mpi_init_thread_exit();
	instr_mpi_init_thread_exit();

	instr_end();

	return 0;
}
