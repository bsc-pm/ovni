/* Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <ovni.h>
#include "instr_nosv.h"

/* Test that we can request the -b flag but still have threads that don't have
 * the nosv.can_breakdown attribute set to true. OpenMP may enable the breakdown
 * on its own. */

int
main(void)
{
	instr_start(0, 1);
	/* Don't enable nosv model via instr_nosv_init() as that would set the
	 * nosv.can_breakdown to 1 */
	instr_require("nosv");
	ovni_attr_set_boolean("nosv.can_breakdown", 0);

	/* Emit a nosv event */
	instr_nosv_type_create(666);

	instr_end();

	return 0;
}
