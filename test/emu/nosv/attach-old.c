/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "compat.h"
#include "ovni.h"
#include "instr.h"
#include "instr_nosv.h"

/* Checks that we can still process a trace which contains the VH{aA} attach
 * events, by simply ignoring them */

int
main(void)
{
	instr_start(0, 1);
	ovni_thread_require("nosv", "1.0.0");

	instr_nosv_attached();
	sleep_us(100);
	instr_nosv_detached();

	instr_end();

	return 0;
}
