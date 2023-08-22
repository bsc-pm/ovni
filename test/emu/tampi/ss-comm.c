/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include "compat.h"
#include "instr.h"
#include "instr_tampi.h"

int
main(void)
{
	/* Test that simulates a communication task executing TAMPI operations */

	const int rank = atoi(getenv("OVNI_RANK"));
	const int nranks = atoi(getenv("OVNI_NRANKS"));

	instr_start(rank, nranks);

	const int ncomms = 100;

	/* Simulate multiple task-aware communications */
	for (int c = 0; c < ncomms; c++) {
		instr_tampi_library_interface_enter();

		/* Issue the non-blocking MPI operation and test it */
		instr_tampi_issue_nonblk_op_enter();
		sleep_us(100);
		instr_tampi_issue_nonblk_op_exit();
		instr_tampi_test_request_enter();
		sleep_us(10);
		instr_tampi_test_request_exit();

		/* Create a ticket if it was not completed */
		if (c % 2 == 0) {
			instr_tampi_create_ticket_enter();
			instr_tampi_create_ticket_exit();

			instr_tampi_add_queues_enter();
			instr_tampi_add_queues_exit();

			/* Wait the ticket if the operation was blocking */
			if (c % 4 == 0) {
				instr_tampi_wait_ticket_enter();
				sleep_us(100);
				instr_tampi_wait_ticket_exit();
			}
		}

		instr_tampi_library_interface_exit();
	}

	instr_end();

	return 0;
}
