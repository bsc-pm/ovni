/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <ovni.h>
#include "instr_nosv.h"
#include "../kernel/instr_kernel.h"

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();
	instr_kernel_init();

	/* Make the current thread out of CPU */
	instr_kernel_cs_out();

	/* Try to emit a nOS-V event: should fail */
	instr_nosv_type_create(666);

	instr_end();

	return 0;
}
