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
	prv_open(&prv, NROWS, path);

	for (int i = 0; i < NROWS; i++) {
		char buf[MAX_CHAN_NAME];
		sprintf(buf, "testchan.%d", i);
		chan_init(&chan[i], CHAN_SINGLE, buf);

		if (bay_register(&bay, &chan[i]) != 0)
			die("bay_register failed\n");
	}

	for (int i = 0; i < NROWS; i++)
		if (prv_register(&prv, i, type, &bay, &chan[i], 0) != 0)
			die("prv_register failed\n");

	for (int i = 0; i < NROWS; i++)
		if (chan_set(&chan[i], value_int64(value_base + i)) != 0)
			die("chan_set failed\n");

	prv_advance(&prv, 10000);

	if (bay_propagate(&bay) != 0)
		die("bay_propagate failed\n");

	/* Ensure all writes are flushed to the buffer and
	 * the header has been fixed */
	prv_close(&prv);

	FILE *f = fopen(path, "r");
	int c;
	while ((c = fgetc(f)) != EOF) {
		fputc(c, stderr);
	}
	fclose(f);

	err("test emit OK\n");
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
	prv_open(&prv, NROWS, path);

	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");

	/* Allow setting the same value in the channel */
	chan_prop_set(&chan, CHAN_ALLOW_DUP, 1);

	if (bay_register(&bay, &chan) != 0)
		die("bay_register failed\n");

	if (prv_register(&prv, 0, type, &bay, &chan, 0) != 0)
		die("prv_register failed\n");

	if (chan_set(&chan, value_int64(1000)) != 0)
		die("chan_set failed\n");

	if (prv_advance(&prv, 10000) != 0)
		die("prv_advance failed\n");

	if (bay_propagate(&bay) != 0)
		die("bay_propagate failed\n");

	/* Set the same value again, which shouldn't fail */
	if (chan_set(&chan, value_int64(1000)) != 0)
		die("chan_set failed\n");

	/* Now the propagation phase must fail, as we cannot write the same
	 * value in the prv trace */
	if (bay_propagate(&bay) == 0)
		die("bay_propagate didn't fail\n");

	/* Ensure all writes are flushed to the buffer and
	 * the header has been fixed */
	prv_close(&prv);

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
	prv_open(&prv, NROWS, path);

	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");

	/* Allow setting the same value in the channel */
	chan_prop_set(&chan, CHAN_ALLOW_DUP, 1);

	OK(bay_register(&bay, &chan));
	OK(prv_register(&prv, row, type, &bay, &chan, 0));
	ERR(prv_register(&prv, row, type, &bay, &chan, 0));

	err("ok");
}

int main(void)
{
	/* Create temporary trace file */
	char fname[] = "/tmp/ovni.prv.XXXXXX";
	int fd = mkstemp(fname);
	if (fd < 0)
		die("mkstemp failed\n");

	test_emit(fname);
	test_duplicate(fname);
	test_same_type(fname);

	close(fd);

	return 0;
}
