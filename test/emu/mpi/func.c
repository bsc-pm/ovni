/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include "compat.h"
#include "instr.h"
#include "instr_mpi.h"

int
main(void)
{
	/* Test that simulates the execution of MPI functions */

	const int rank = atoi(getenv("OVNI_RANK"));
	const int nranks = atoi(getenv("OVNI_NRANKS"));

	instr_start(rank, nranks);
	instr_mpi_init();

	/* Initialize MPI */
	instr_mpi_init_thread_enter();
	instr_mpi_init_thread_exit();

	/* Issue a non-blocking broadcast */
	instr_mpi_ibcast_enter();
	instr_mpi_ibcast_exit();

	/* Wait the broadcast request */
	instr_mpi_wait_enter();
	instr_mpi_wait_exit();

	/* Perform a barrier */
	instr_mpi_barrier_enter();
	instr_mpi_barrier_exit();

	const int ncomms = 100;

	/* Simulate multiple nonb-blocking communications */
	for (int c = 0; c < ncomms; c++) {
		/* Issue a non-blocking synchronous send */
		instr_mpi_issend_enter();
		instr_mpi_issend_exit();

		/* Issue a non-blocking receive */
		instr_mpi_irecv_enter();
		instr_mpi_irecv_exit();
	}

	/* Simulate testsome calls until all communications are completed */
	for (int c = 0; c < ncomms; c++) {
		instr_mpi_testsome_enter();
		instr_mpi_testsome_exit();
	}

	/* Allreduce a value */
	instr_mpi_allreduce_enter();
	instr_mpi_allreduce_exit();

	/* Finalize MPI */
	instr_mpi_finalize_enter();
	instr_mpi_finalize_exit();

	instr_end();

	return 0;
}
