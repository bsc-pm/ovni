/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "ovni.h"

/* Test the case in which events from a secondary unsorted region are
 * introduced before the previous unsorted region:
 *
 * [bbbb][BBbb]
 * bbbbBB[]bb[]
 *
 * The events B contain 16 bytes of payload, so pointer to events in previous
 * positions are now pointing to garbage.
 *
 * See: https://pm.bsc.es/gitlab/rarias/ovni/-/issues/199
 */

static void
emit(char *mcv, uint64_t clock, int size)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, mcv);
	ovni_ev_set_clock(&ev, clock);

	if (size) {
		uint8_t buf[64] = { 0 };
		ovni_payload_add(&ev, buf, size);
	}

	ovni_ev_emit(&ev);
}

int
main(void)
{
	set_clock(1);
	instr_start(0, 1);

	/* Leave some room to prevent clashes */
	uint64_t t0 = 100;

	/* We want it to end like this:
	 *
	 * t0  +1  +2  +3  +4  +5  +6  +7  +8  +9  +10 +11
	 * b   b   b   b   B   B   [   ]   b   b   [   ]
	 *
	 * But we emit it like this:
	 *
	 * +6  +0  +1  +2  +3  +7  +10 +4  +5  +8  +9  +11
	 * [   b   b   b   b   ]   [   B   B   b   b   ]
	 */

	emit("OU[", t0 + 6, 0);
	emit("OB.", t0 + 0, 0);
	emit("OB.", t0 + 1, 0);
	emit("OB.", t0 + 2, 0);
	emit("OB.", t0 + 3, 0);
	emit("OU]", t0 + 7, 0);

	emit("OU[", t0 + 10, 0);
	emit("OB.", t0 + 4, 16);
	emit("OB.", t0 + 5, 16);
	emit("OB.", t0 + 8, 0);
	emit("OB.", t0 + 9, 0);
	emit("OU]", t0 + 11, 0);

	set_clock(200);
	instr_end();

	return 0;
}
