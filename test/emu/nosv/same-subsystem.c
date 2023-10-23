/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"

/* With the introduction of ovni.level in nOS-V, we can have the
 * situation in which two VTx events are emitted without the subsystem
 * events. This causes the subsystem channel to push twice the same
 * value ST_TASK_BODY. */

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	instr_nosv_type_create(10);
	instr_nosv_task_create(1, 10);
	instr_nosv_task_create(2, 10);
	instr_nosv_task_execute(1, 0);
	instr_nosv_task_pause(1, 0);
	instr_nosv_task_execute(2, 0);
	instr_nosv_task_end(2, 0);
	instr_nosv_task_resume(1, 0);
	instr_nosv_task_end(1, 0);

	instr_end();

	return 0;
}
