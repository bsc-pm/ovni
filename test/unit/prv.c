/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "emu/bay.h"
#include "emu/chan.h"
#include "emu/pv/prv.h"
#include "unittest.h"
#include "value.h"

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
		chan_init(&chan[i], CHAN_SINGLE, "testchan.%d", i);
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
test_dup(const char *path)
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

	err("OK");
}

static int
count_prv_lines(char *fpath, int64_t time, int row0, long type, long value)
{
	int count = 0;
	char line[1024];

	FILE *f = fopen(fpath, "r");

	if (f == NULL)
		die("fopen failed:");

	while (fgets(line, 1024, f) != NULL) {
		if (line[0] != '2')
			continue;

		int64_t ftime;
		long frow1, ftype, fvalue;
		int ret = sscanf(line, "2:0:1:1:%ld:%" SCNd64 ":%ld:%ld",
				&frow1, &ftime, &ftype, &fvalue);

		if (ret != 4)
			die("ret=%d", ret);

		if (row0 + 1 != frow1 || time != ftime)
			continue;

		if (type != ftype || value != fvalue)
			continue;

		count++;
	}

	fclose(f);

	return count;
}

static void
test_skipdup(char *fname)
{
	/* Test PRV_SKIPDUP flag (skip duplicates) */
	FILE *wf = fopen(fname, "w");

	if (wf == NULL)
		die("fmemopen failed:");

	int64_t time = 0;
	long type = 100;
	long value = 1000;

	struct bay bay;
	bay_init(&bay);

	struct prv prv;
	OK(prv_open_file(&prv, NROWS, wf));

	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");

	/* Allow setting the same value in the channel */
	chan_prop_set(&chan, CHAN_ALLOW_DUP, 1);

	OK(bay_register(&bay, &chan));
	OK(prv_register(&prv, 0, type, &bay, &chan, PRV_SKIPDUP));
	time += 10000;

	/* Set the initial value */
	OK(chan_set(&chan, value_int64(value)));
	OK(prv_advance(&prv, time));

	/* Propagate will emit the value into the PRV */
	OK(bay_propagate(&bay));
	fflush(wf);

	/* Check for the line */
	if (count_prv_lines(fname, time, 0, type, value) != 1)
		die("expected line not found or multiple matches");

	/* Set the same value again, which shouldn't fail */
	OK(chan_set(&chan, value_int64(value)));

	/* Propagate again, emitting the value */
	OK(bay_propagate(&bay));
	fflush(wf);

	/* Ensure that we didn't write it again */
	if (count_prv_lines(fname, time, 0, type, value) != 1)
		die("expected line not found or multiple matches");

	fclose(wf);

	err("OK");
}

static void
test_emitdup(char *fname)
{
	/* Test PRV_EMITDUP flag (emit duplicates) */
	FILE *wf = fopen(fname, "w");

	if (wf == NULL)
		die("fopen failed:");

	int64_t time = 0;
	long type = 100;
	long value = 1000;

	struct bay bay;
	bay_init(&bay);

	struct prv prv;
	OK(prv_open_file(&prv, NROWS, wf));

	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");

	/* Allow setting the same value in the channel */
	chan_prop_set(&chan, CHAN_ALLOW_DUP, 1);

	OK(bay_register(&bay, &chan));
	OK(prv_register(&prv, 0, type, &bay, &chan, PRV_EMITDUP));
	time += 10000;

	/* Set the initial value */
	OK(chan_set(&chan, value_int64(value)));
	OK(prv_advance(&prv, time));

	/* Propagate will emit the value into the PRV */
	OK(bay_propagate(&bay));
	fflush(wf);

	/* Check for the line */
	if (count_prv_lines(fname, time, 0, type, value) != 1)
		die("expected line not found or multiple matches");

	/* Set the same value again, which shouldn't fail */
	OK(chan_set(&chan, value_int64(value)));

	/* Propagate again, emitting the value */
	OK(bay_propagate(&bay));
	fflush(wf);

	/* Ensure that we write it again */
	if (count_prv_lines(fname, time, 0, type, value) != 2)
		die("expected line not found or multiple matches");

	fclose(wf);

	err("OK");
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
	char fname[] = "ovni.prv";

	test_emit(fname);
	test_dup(fname);
	test_skipdup(fname);
	test_emitdup(fname);
	test_same_type(fname);

	return 0;
}
