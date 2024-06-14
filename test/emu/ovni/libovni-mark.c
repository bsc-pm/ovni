/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <string.h>
#include "instr.h"
#include "ovni.h"

enum {
	MARK_COLORS = 1,
	COL_BLUE = 1,
	COL_GRAY = 2,
	COL_RED = 3,
};

/* Check ovni_mark_* API. */
int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));
	instr_start(rank, nranks);

	ovni_mark_type(MARK_COLORS, OVNI_MARK_STACK, "Colors");

	if(!ovni_attr_has("ovni.mark.1.title"))
		die("missing mark title");

	ovni_mark_label(MARK_COLORS, COL_BLUE, "Blue");
	ovni_mark_label(MARK_COLORS, COL_GRAY, "Gray");
	ovni_mark_label(MARK_COLORS, COL_RED,  "Red");

	sleep_us(10); ovni_mark_push(MARK_COLORS, COL_BLUE);
	sleep_us(10); ovni_mark_push(MARK_COLORS, COL_GRAY);
	sleep_us(10); ovni_mark_push(MARK_COLORS, COL_RED);
	sleep_us(50); ovni_mark_pop(MARK_COLORS, COL_RED);
	sleep_us(10); ovni_mark_pop(MARK_COLORS, COL_GRAY);
	sleep_us(10); ovni_mark_pop(MARK_COLORS, COL_BLUE);

	instr_end();

	return 0;
}
