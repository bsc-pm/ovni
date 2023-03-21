/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu/bay.h"
#include "common.h"
#include "unittest.h"

static void
test_duplicate(struct bay *bay)
{
	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "dup");

	OK(bay_register(bay, &chan));
	ERR(bay_register(bay, &chan));
}

static int
callback(struct chan *chan, void *ptr)
{
	struct value value;
	OK(chan_read(chan, &value));

	if (value.type != VALUE_INT64)
		die("unexpected value type");

	int64_t *ival = ptr;
	*ival = value.i;

	return 0;
}

static void
test_callback(struct bay *bay)
{
	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");

	OK(bay_register(bay, &chan));

	int64_t data = 0;
	if (bay_add_cb(bay, BAY_CB_DIRTY, &chan, callback, &data, 1) == NULL)
		die("bay_add_cb failed");

	if (data != 0)
		die("data changed after bay_chan_append_cb");

	OK(chan_set(&chan, value_int64(1)));

	if (data != 0)
		die("data changed after chan_set");

	/* Now the callback should modify 'data' */
	OK(bay_propagate(bay));

	if (data != 1)
		die("data didn't change after bay_propagate");
}

int main(void)
{
	struct bay bay;
	bay_init(&bay);

	test_duplicate(&bay);
	test_callback(&bay);

	return 0;
}
