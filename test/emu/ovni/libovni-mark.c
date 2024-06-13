/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <string.h>
#include "instr.h"
#include "ovni.h"

enum {
	MARK_COLORS = 1,
	COL_RED = 1,
	COL_BLUE = 2,
	COL_GREEN = 3,
};

/* Check ovni_mark_* API. */
int
main(void)
{
	instr_start(0, 1);

	ovni_mark_type(MARK_COLORS, "Colors");

	if(!ovni_attr_has("ovni.mark.1.title"))
		die("missing mark title");

	ovni_mark_label(MARK_COLORS, 1, "Red");
	ovni_mark_label(MARK_COLORS, 2, "Blue");
	ovni_mark_label(MARK_COLORS, 3, "Green");

	sleep_us(10); ovni_mark_push(MARK_COLORS, COL_RED);
	sleep_us(10); ovni_mark_push(MARK_COLORS, COL_BLUE);
	sleep_us(10); ovni_mark_push(MARK_COLORS, COL_GREEN);
	sleep_us(10); ovni_mark_pop(MARK_COLORS, COL_GREEN);
	sleep_us(10); ovni_mark_pop(MARK_COLORS, COL_BLUE);
	sleep_us(10); ovni_mark_pop(MARK_COLORS, COL_RED);

	instr_end();

	return 0;
}
