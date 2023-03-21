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
count_prv_lines(FILE *f, int64_t time, int row0, long type, long value)
{
	int count = 0;
	char line[1024];

	rewind(f);

	while (fgets(line, 1024, f) != NULL) {
		if (line[0] != '2')
			continue;

		int64_t ftime;
		long frow1, ftype, fvalue;
		int ret = sscanf(line, "2:0:1:1:%ld:%ld:%ld:%ld",
				&frow1, &ftime, &ftype, &fvalue);

		if (ret != 4)
			die("ret=%d", ret);

		if (row0 + 1 != frow1 || time != ftime)
			continue;

		if (type != ftype || value != fvalue)
			continue;

		count++;
	}

	return count;
}

static void
test_skipdup(void)
{
	/* Test PRV_SKIPDUP flag (skip duplicates) */

	size_t size = 1024;
	char *buf = calloc(1, size);

	if (buf == NULL)
		die("calloc failed:");

	FILE *wf = fmemopen(buf, size, "w");

	if (wf == NULL)
		die("fmemopen failed:");

	FILE *rf = fmemopen(buf, size, "r");

	if (rf == NULL)
		die("fmemopen failed:");

	/* Disable buffering */
	setbuf(wf, NULL);
	setbuf(rf, NULL);

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

	/* Check for the line */
	if (count_prv_lines(rf, time, 0, type, value) != 1)
		die("expected line not found or multiple matches");

	/* Set the same value again, which shouldn't fail */
	OK(chan_set(&chan, value_int64(value)));

	/* Propagate again, emitting the value */
	OK(bay_propagate(&bay));

	/* Ensure that we didn't write it again */
	if (count_prv_lines(rf, time, 0, type, value) != 1)
		die("expected line not found or multiple matches");

	fclose(wf);
	fclose(rf);

	err("OK");
}

static void
test_emitdup(void)
{
	/* Test PRV_EMITDUP flag (emit duplicates) */

	size_t size = 1024;
	char *buf = calloc(1, size);

	if (buf == NULL)
		die("calloc failed:");

	FILE *wf = fmemopen(buf, size, "w");

	if (wf == NULL)
		die("fmemopen failed:");

	FILE *rf = fmemopen(buf, size, "r");

	if (rf == NULL)
		die("fmemopen failed:");

	/* Disable buffering */
	setbuf(wf, NULL);
	setbuf(rf, NULL);

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

	/* Check for the line */
	if (count_prv_lines(rf, time, 0, type, value) != 1)
		die("expected line not found or multiple matches");

	/* Set the same value again, which shouldn't fail */
	OK(chan_set(&chan, value_int64(value)));

	/* Propagate again, emitting the value */
	OK(bay_propagate(&bay));

	/* Ensure that we write it again */
	if (count_prv_lines(rf, time, 0, type, value) != 2)
		die("expected line not found or multiple matches");

	fclose(wf);
	fclose(rf);

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
	/* Create temporary trace file */
	char fname[] = "/tmp/ovni.prv.XXXXXX";
	int fd = mkstemp(fname);
	if (fd < 0)
		die("mkstemp failed:");

	test_emit(fname);
	test_dup(fname);
	test_skipdup();
	test_emitdup();
	test_same_type(fname);

	close(fd);

	return 0;
}
