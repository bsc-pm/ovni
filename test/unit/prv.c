/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _GNU_SOURCE

#include "unittest.h"

#include "emu/pv/prv.h"
#include "common.h"

#include <stdio.h>
#include <unistd.h>

#define NROWS 10

static void
test_emit(const char *path)
{
	long type = 100;
	long value_base = 1000;
	struct chan chan[NROWS];

	struct bay bay;
	bay_init(&bay);

	struct prv prv;
	OK(prv_open(&prv, NROWS, path));

	for (int i = 0; i < NROWS; i++) {
		char buf[MAX_CHAN_NAME];
		sprintf(buf, "testchan.%d", i);
		chan_init(&chan[i], CHAN_SINGLE, buf);

		OK(bay_register(&bay, &chan[i]));
	}

	for (int i = 0; i < NROWS; i++)
		OK(prv_register(&prv, i, type, &bay, &chan[i], 0));

	for (int i = 0; i < NROWS; i++)
		OK(chan_set(&chan[i], value_int64(value_base + i)));

	OK(prv_advance(&prv, 10000));

	OK(bay_propagate(&bay));

	/* Ensure all writes are flushed to the buffer and
	 * the header has been fixed */
	OK(prv_close(&prv));

	FILE *f = fopen(path, "r");
	int c;
	while ((c = fgetc(f)) != EOF) {
		fputc(c, stderr);
	}
	fclose(f);

	err("OK");
}

static void
test_duplicate(const char *path)
{
	/* Ensure that we detect duplicate values being emitted in the Paraver
	 * trace */

	long type = 100;

	struct bay bay;
	bay_init(&bay);

	struct prv prv;
	OK(prv_open(&prv, NROWS, path));

	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");

	/* Allow setting the same value in the channel */
	chan_prop_set(&chan, CHAN_ALLOW_DUP, 1);

	OK(bay_register(&bay, &chan));
	OK(prv_register(&prv, 0, type, &bay, &chan, 0));
	OK(chan_set(&chan, value_int64(1000)));
	OK(prv_advance(&prv, 10000));
	OK(bay_propagate(&bay));

	/* Set the same value again, which shouldn't fail */
	OK(chan_set(&chan, value_int64(1000)));

	/* Now the propagation phase must fail, as we cannot write the same
	 * value in the prv trace */
	ERR(bay_propagate(&bay));

	/* Ensure all writes are flushed to the buffer and
	 * the header has been fixed */
	OK(prv_close(&prv));

	err("test duplicate OK\n");
}

/* Detect same types being registered to the same row */
static void
test_same_type(const char *path)
{
	long type = 100;
	long row = 0;

	struct bay bay;
	bay_init(&bay);

	struct prv prv;
	OK(prv_open(&prv, NROWS, path));

	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");

	/* Allow setting the same value in the channel */
	chan_prop_set(&chan, CHAN_ALLOW_DUP, 1);

	OK(bay_register(&bay, &chan));
	OK(prv_register(&prv, row, type, &bay, &chan, 0));
	ERR(prv_register(&prv, row, type, &bay, &chan, 0));

	err("OK");
}

int main(void)
{
	/* Create temporary trace file */
	char fname[] = "/tmp/ovni.prv.XXXXXX";
	int fd = mkstemp(fname);
	if (fd < 0)
		die("mkstemp failed:");

	test_emit(fname);
	test_duplicate(fname);
	test_same_type(fname);

	close(fd);

	return 0;
}
