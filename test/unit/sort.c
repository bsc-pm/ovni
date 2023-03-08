/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu/sort.h"
#include "common.h"
#include "unittest.h"

#define N 10

static void
check_output(struct chan *chan, struct value expected)
{
	struct value out_value = value_null();
	if (chan_read(chan, &out_value) != 0)
		die("chan_read() failed for channel %s", chan->name);

	char buf1[128];
	if (!value_is_equal(&out_value, &expected)) {
		char buf2[128];
		die("unexpected value found %s in output (expected %s)\n",
				value_str(out_value, buf1),
				value_str(expected, buf2));
	}

	err("output ok: chan=%s val=%s", chan->name, value_str(out_value, buf1));
}

static void
test_sort(void)
{
	struct bay bay;
	bay_init(&bay);

	struct chan inputs[N];

	for (int i = 0; i < N; i++)
		chan_init(&inputs[i], CHAN_SINGLE, "input.%d", i);

	/* Register all channels in the bay */
	for (int i = 0; i < N; i++)
		OK(bay_register(&bay, &inputs[i]));

	/* Setup channel values */
	for (int i = 0; i < N; i++)
		OK(chan_set(&inputs[i], value_int64(0)));

	OK(bay_propagate(&bay));

	struct sort sort;
	OK(sort_init(&sort, &bay, N, "sort0"));

	for (int i = 0; i < N; i++)
		OK(sort_set_input(&sort, i, &inputs[i]));

	for (int i = 0; i < 2; i++)
		OK(chan_set(&inputs[i], value_int64(1)));

	OK(bay_propagate(&bay));

	/* Check the outputs */
	for (int i = 0; i < N - 2; i++) {
		struct chan *out = sort_get_output(&sort, i);
		check_output(out, value_int64(0));
	}
	for (int i = N - 2; i < N; i++) {
		struct chan *out = sort_get_output(&sort, i);
		check_output(out, value_int64(1));
	}
}

int
main(void)
{
	test_sort();

	err("OK\n");

	return 0;
}
