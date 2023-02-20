/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "instr_nanos6.h"

int
main(void)
{
	instr_start(0, 1);

	/* Until here, the nanos6 model has not been connected to the
	 * patchbay yet. The next event will cause the subsystem mux to
	 * be connected to the patchbay. We need to ensure that at that
	 * moment the select channel is evaluated, otherwise the mux
	 * will remain selecting a null input until the thread state
	 * changes. */

	/* FIXME: We should be able to test that after emitting the
	 * nanos6 event the emulator follows some properties. */

	instr_nanos6_worker_loop_enter();

	instr_nanos6_worker_loop_exit();

	instr_end();

	return 0;
}
