/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

//#define ENABLE_DEBUG

#include "prv.h"
#include <stdio.h>
#include <errno.h>

static void
write_header(FILE *f, long long duration, int nrows)
{
	fprintf(f, "#Paraver (19/01/38 at 03:14):%020lld_ns:0:1:1(%d:1)\n",
			duration, nrows);
}

int
prv_open_file(struct prv *prv, long nrows, FILE *file)
{
	memset(prv, 0, sizeof(struct prv));

	prv->nrows = nrows;
	prv->file = file;

	/* Write fake header to allocate the space */
	write_header(file, 0LL, nrows);

	return 0;
}

int
prv_open(struct prv *prv, long nrows, const char *path)
{
	FILE *f = fopen(path, "w");

	if (f == NULL) {
		die("cannot open file '%s' for writting: %s\n",
				path, strerror(errno));
		return -1;
	}

	return prv_open_file(prv, nrows, f);
}

void
prv_close(struct prv *prv)
{
	/* Fix the header with the current duration */
	fseek(prv->file, 0, SEEK_SET);
	write_header(prv->file, prv->time, prv->nrows);
	fclose(prv->file);
}

static struct prv_chan *
find_prv_chan(struct prv *prv, const char *name)
{
	struct prv_chan *rchan = NULL;
	HASH_FIND_STR(prv->channels, name, rchan);

	return rchan;
}

static int
write_line(struct prv *prv, long row_base1, long type, long value)
{
	int ret = fprintf(prv->file, "2:0:1:1:%ld:%ld:%ld:%ld\n",
			row_base1, prv->time, type, value);

	if (ret < 0)
		return -1;
	else
		return 0;
}

static int
emit(struct prv *prv, struct prv_chan *rchan)
{
	struct value value;
	struct chan *chan = rchan->chan;
	char buf[128];
	if (chan_read(chan, &value) != 0) {
		err("chan_read %s failed\n", chan->name);
		return -1;
	}

	/* Ensure we don't emit the same value twice */
	if (rchan->last_value_set && value_is_equal(&value, &rchan->last_value)) {
		char buf[128];
		if (rchan->flags & PRV_DUP) {
			dbg("skipping duplicated value %s for channel %s\n",
					value_str(value, buf), chan->name);
			return 0;
		} else {
			err("error duplicated value %s for channel %s\n",
					value_str(value, buf), chan->name);
			return -1;
		}
	}

	long val = 0;
	switch (value.type) {
		case VALUE_INT64:
			val = value.i;
			//if (val == 0) {
			//	err("forbidden value 0 in %s: %s\n",
			//			chan->name,
			//			value_str(value, buf));
			//	return -1;
			//}
			break;
		case VALUE_NULL:
			val = 0;
			break;
		default:
			err("chan_read %s only int64 and null supported: %s\n",
					chan->name, value_str(value, buf));
			return -1;
	}

	if (write_line(prv, rchan->row_base1, rchan->type, val) != 0) {
		err("write_line failed for channel %s\n",
				chan->name);
		return -1;
	}

	dbg("written %s for chan %s", value_str(value, buf), chan->name);

	rchan->last_value = value;
	rchan->last_value_set = 1;

	return 0;
}

static int
cb_prv(struct chan *chan, void *ptr)
{
	UNUSED(chan);
	struct prv_chan *rchan = ptr;
	struct prv *prv = rchan->prv;

	return emit(prv, rchan);
}

int
prv_register(struct prv *prv, long row, long type, struct bay *bay, struct chan *chan, long flags)
{
	/* FIXME: use the type instead of channel name as key */
	struct prv_chan *rchan = find_prv_chan(prv, chan->name);
	if (rchan != NULL) {
		err("channel %s already registered", chan->name);
		return -1;
	}

	rchan = calloc(1, sizeof(struct prv_chan));
	if (rchan == NULL) {
		err("calloc failed:");
		return -1;
	}

	rchan->chan = chan;
	rchan->row_base1 = row + 1;
	rchan->type = type;
	rchan->prv = prv;
	rchan->last_value = value_null();
	rchan->last_value_set = 0;
	rchan->flags = flags;

	/* Add emit callback */
	if (bay_add_cb(bay, BAY_CB_EMIT, chan, cb_prv, rchan) != 0) {
		err("bay_add_cb failed");
		return -1;
	}

	/* Add to hash table */
	HASH_ADD_STR(prv->channels, chan->name, rchan);

	return 0;
}

int
prv_advance(struct prv *prv, int64_t time)
{
	if (time < prv->time) {
		err("cannot move to previous time");
		return -1;
	}

	prv->time = time;
	return 0;
}
