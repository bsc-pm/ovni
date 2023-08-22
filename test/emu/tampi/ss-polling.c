/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdlib.h>
#include "compat.h"
#include "instr.h"
#include "instr_tampi.h"

int
main(void)
{
	/* Test that simulates the polling task of TAMPI */

	const int rank = atoi(getenv("OVNI_RANK"));
	const int nranks = atoi(getenv("OVNI_NRANKS"));

	instr_start(rank, nranks);

	const int transfer_step = 5;
	const int complete_step = 3;

	int nreqs_to_transfer = 100;
	int nreqs_to_test = 0;
	int t, c;

	/* Simulate the loop of the polling task */
	while (nreqs_to_transfer || nreqs_to_test) {
		instr_tampi_library_polling_enter();

		t = 0;
		instr_tampi_transfer_queues_enter();
		while (nreqs_to_transfer && t < transfer_step) {
			/* Transfer a request/ticket to the global array */
			--nreqs_to_transfer;
			++nreqs_to_test;
			++t;
		}
		instr_tampi_transfer_queues_exit();

		/* Check the global array of requests/tickets */
		if (nreqs_to_test) {
			instr_tampi_check_global_array_enter();

			/* Testsome requests */
			instr_tampi_testsome_requests_enter();
			sleep_us(10);
			instr_tampi_testsome_requests_exit();

			c = 0;
			while (nreqs_to_test && c < complete_step) {
				/* Process a completed request */
				instr_tampi_completed_request_enter();
				sleep_us(1);
				instr_tampi_completed_request_exit();
				--nreqs_to_test;
				++c;
			}
			instr_tampi_check_global_array_exit();
		}

		instr_tampi_library_polling_exit();

		sleep_us(100);
	}

	instr_end();

	return 0;
}
