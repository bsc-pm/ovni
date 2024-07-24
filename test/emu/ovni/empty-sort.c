/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "ovni.h"

static void
emit(char *mcv, int64_t clock)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, mcv);
	ovni_ev_set_clock(&ev, (uint64_t) clock);
	ovni_ev_emit(&ev);
}

int
main(void)
{
	instr_start(0, 1);

	/* Leave some room to prevent clashes */
	sleep_us(100); /* 100000 us */

	int64_t t0 = (int64_t) ovni_clock_now();

	emit("OU[", t0);

	int64_t t = t0 - 10000;
	for (int i = 0; i < 100; i++) {
		emit("OB.", t);
		t += 33;
	}

	emit("OU]", (int64_t) ovni_clock_now());

	instr_end();

	return 0;
}
