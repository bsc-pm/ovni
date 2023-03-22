/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "ovni.h"

static void
emit(char *mcv, uint64_t clock)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, mcv);
	ovni_ev_set_clock(&ev, clock);
	ovni_ev_emit(&ev);
}

int
main(void)
{
	instr_start(0, 1);

	uint64_t t = ovni_clock_now();

	/* Emit bursts with controlled clocks */
	for (int i = 0; i < 100; i++) {
		emit("OB.", t);
		t += 33;
	}

	/* Sleep a bit to prevent unsorted events */
	while (ovni_clock_now() < t)
		sleep_us(10);

	instr_end();

	return 0;
}
