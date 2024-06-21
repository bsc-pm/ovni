/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu/mux.h"
#include <stdio.h>
#include "common.h"
#include "emu/bay.h"
#include "emu/chan.h"
#include "unittest.h"

#define N 10

static void
check_output(struct mux *mux, struct value expected)
{
	struct value out_value = value_null();
	OK(chan_read(mux->output, &out_value));

	if (!value_is_equal(&out_value, &expected)) {
		die("unexpected value found %s in output (expected %s)",
				value_str(out_value),
				value_str(expected));
	}
}

static void
test_select(struct mux *mux, int key)
{
	OK(chan_set(mux->select, value_int64(key)));
	OK(bay_propagate(mux->bay));
	check_output(mux, value_int64(1000 + key));
	err("OK");
}

static void
test_input(struct mux *mux, int key)
{
	/* Set the select channel to the selected key */
	test_select(mux, key);

	int new_value = 2000 + key;

	/* Then change that channel */
	struct mux_input *mi = mux_get_input(mux, key);
	if (mi == NULL)
		die("mux_get_input failed to locate input %d", key);

	OK(chan_set(mi->chan, value_int64(new_value)));
	OK(bay_propagate(mux->bay));
	check_output(mux, value_int64(new_value));
	err("OK");
}

static void
test_select_and_input(struct mux *mux, int key)
{
	/* Set the select channel to the selected key, but don't
	 * propagate the changes yet */
	OK(chan_set(mux->select, value_int64(key)));

	int new_value = 2000 + key;

	/* Also change that channel */
	struct mux_input *mi = mux_get_input(mux, key);
	if (mi == NULL)
		die("mux_get_input failed to locate input %d", key);

	OK(chan_set(mi->chan, value_int64(new_value)));

	/* Write twice to the output */
	OK(bay_propagate(mux->bay));

	check_output(mux, value_int64(new_value));
	err("OK");
}

static void
test_input_and_select(struct mux *mux, int key)
{
	int new_value = 2000 + key;

	/* First change the input */
	struct mux_input *mi = mux_get_input(mux, key);
	if (mi == NULL)
		die("mux_get_input failed to locate input %d", key);

	OK(chan_set(mi->chan, value_int64(new_value)));

	/* Then change select */
	OK(chan_set(mux->select, value_int64(key)));

	/* Write twice to the output */
	OK(bay_propagate(mux->bay));

	check_output(mux, value_int64(new_value));
	err("OK");
}

static void
test_mid_propagate(struct mux *mux, int key)
{
	int new_value = 2000 + key;

	struct mux_input *mi = mux_get_input(mux, key);
	if (mi == NULL)
		die("mux_get_input failed to locate input %d", key);

	OK(chan_set(mi->chan, value_int64(new_value)));
	OK(bay_propagate(mux->bay));
	OK(chan_set(mux->select, value_int64(key)));
	OK(bay_propagate(mux->bay));

	check_output(mux, value_int64(new_value));
	err("OK");
}

static void
test_duplicate_output(struct mux *mux, int key1, int key2)
{
	int new_value = 2000 + key1;

	struct mux_input *in1 = mux_get_input(mux, key1);
	if (in1 == NULL)
		die("mux_get_input failed to locate input1 %d", key1);

	struct mux_input *in2 = mux_get_input(mux, key2);
	if (in2 == NULL)
		die("mux_get_input failed to locate input2 %d", key2);

	OK(chan_set(in1->chan, value_int64(new_value)));
	OK(chan_set(in2->chan, value_int64(new_value)));

	/* Select input 1 */
	OK(chan_set(mux->select, value_int64(key1)));
	OK(bay_propagate(mux->bay));

	check_output(mux, value_int64(new_value));

	/* Now switch to input 2, which has the same value */
	OK(chan_set(mux->select, value_int64(key2)));
	OK(bay_propagate(mux->bay));

	check_output(mux, value_int64(new_value));
	err("OK");
}

int
main(void)
{
	struct bay bay;
	bay_init(&bay);

	struct chan inputs[N];
	struct chan output;
	struct chan select;

	chan_init(&output, CHAN_SINGLE, "output");
	chan_init(&select, CHAN_SINGLE, "select");

	for (int i = 0; i < N; i++)
		chan_init(&inputs[i], CHAN_SINGLE, "input.%d", i);

	/* Register all channels in the bay */
	OK(bay_register(&bay, &select));

	for (int i = 0; i < N; i++)
		OK(bay_register(&bay, &inputs[i]));

	struct mux mux;
	/* Attempt to init the mux without registering the output */
	ERR(mux_init(&mux, &bay, &select, &output, NULL, N));
	OK(bay_register(&bay, &output));
	OK(mux_init(&mux, &bay, &select, &output, NULL, N));

	for (int i = 0; i < N; i++)
		OK(mux_set_input(&mux, i, &inputs[i]));

	/* Write something to the input channels */
	for (int i = 0; i < N; i++)
		OK(chan_set(&inputs[i], value_int64(1000 + i)));

	/* Propagate values and call the callbacks */
	OK(bay_propagate(&bay));

	test_select(&mux, 1);
	test_input(&mux, 2);
	test_select_and_input(&mux, 3);
	test_input_and_select(&mux, 4);
	test_mid_propagate(&mux, 5);
	test_duplicate_output(&mux, 6, 7);

	err("OK");

	return 0;
}
