/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <string.h>
#include "instr.h"
#include "ovni.h"

enum {
	MARK_PROGRESS = 1,
	MARK_COLORS = 2,
	COL_BLUE = 1,
	COL_GRAY = 2,
	COL_RED = 3,
};

static void
rand_sleep(int max_us)
{
	int t = rand() % max_us;
	sleep_us(t);
}

/* Check ovni_mark_* API. */
int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));
	instr_start(rank, nranks);

	/* Deterministic rand() */
	srand(100U + (unsigned) rank);

	/* Test set without labels */

	ovni_mark_type(MARK_PROGRESS, 0, "Progress");

	/* Test push/pop */

	ovni_mark_type(MARK_COLORS, OVNI_MARK_STACK, "Colors");

	if(!ovni_attr_has("ovni.mark.1.title"))
		die("missing mark title");

	ovni_mark_label(MARK_COLORS, COL_BLUE, "Blue");
	ovni_mark_label(MARK_COLORS, COL_GRAY, "Gray");
	ovni_mark_label(MARK_COLORS, COL_RED,  "Red");

	for (int i = 1; i <= 100; i++) {
		int which = 1 + (rand() % 3);

		/* Simulate a thread goes to sleep */
		if (rank == 0 && i == 25)
			instr_thread_pause();

		ovni_mark_push(MARK_COLORS, which);
		rand_sleep(500);
		ovni_mark_pop(MARK_COLORS, which);

		if (rank == 0 && i == 75)
			instr_thread_resume();

		ovni_mark_set(MARK_PROGRESS, i);
	}

	/* Ensure we can emit the same value several times */
	for (int i = 0; i < 10; i++) {
		sleep_us(10);
		ovni_mark_set(MARK_PROGRESS, 100);
	}

	instr_end();

	return 0;
}
