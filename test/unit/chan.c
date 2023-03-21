/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu/chan.h"
#include "common.h"
#include "unittest.h"

static void
test_dirty(void)
{
	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");

	OK(chan_set(&chan, value_int64(1)));

	/* Now channel is dirty */

	ERR(chan_set(&chan, value_int64(2)));

	OK(chan_flush(&chan));

	chan_prop_set(&chan, CHAN_DIRTY_WRITE, 1);

	OK(chan_set(&chan, value_int64(3)));

	/* Now is dirty, but it should allow multiple writes */
	OK(chan_set(&chan, value_int64(4)));

	struct value value;
	struct value four = value_int64(4);

	OK(chan_read(&chan, &value));

	if (!value_is_equal(&value, &four))
		die("chan_read returned unexpected value");

	OK(chan_flush(&chan));

	err("OK");
}

static void
test_single(void)
{
	struct chan chan;
	struct value one = { .type = VALUE_INT64, .i = 1 };
	struct value two = { .type = VALUE_INT64, .i = 2 };

	chan_init(&chan, CHAN_SINGLE, "testchan");

	/* Ensure we cannot push as stack */
	ERR(chan_push(&chan, one));

	/* Now we should be able to write with set */
	OK(chan_set(&chan, one));

	/* Now is dirty, it shouldn't allow another set */
	ERR(chan_set(&chan, two));

	struct value value;

	OK(chan_read(&chan, &value));

	if (!value_is_equal(&value, &one))
		die("chan_read returned unexpected value");

	err("OK");
}

/* Test that writting the same value to a channel raises an error, but not if we
 * set the CHAN_ALLOW_DUP flag */
static void
test_allow_dup(void)
{
	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");

	OK(chan_set(&chan, value_int64(1)));
	OK(chan_flush(&chan));

	/* Attempt to write the same value again */
	ERR(chan_set(&chan, value_int64(1)));

	/* Allow duplicates */
	chan_prop_set(&chan, CHAN_ALLOW_DUP, 1);

	/* Then it should allow writting the same value */
	OK(chan_set(&chan, value_int64(1)));

	/* Also ensure the channel is dirty */
	if (!chan.is_dirty)
		die("chan is not dirty");

	err("OK");
}

/* Test that writting the same value to a channel with the flag CHAN_IGNORE_DUP,
 * causes the channel to remain clean (without the is_dirty flag set) */
static void
test_ignore_dup(void)
{
	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");
	chan_prop_set(&chan, CHAN_IGNORE_DUP, 1);

	OK(chan_set(&chan, value_int64(1)));

	OK(chan_flush(&chan));

	/* Write the same value */
	OK(chan_set(&chan, value_int64(1)));

	/* And ensure the channel is not dirty */
	if (chan.is_dirty)
		die("chan is dirty");

	err("OK");
}

int main(void)
{
	test_single();
	test_dirty();
	test_allow_dup();
	test_ignore_dup();

	return 0;
}
