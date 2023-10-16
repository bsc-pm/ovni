/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PRV_H
#define PRV_H

#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "uthash.h"
#include "value.h"
struct bay;
struct chan;

enum prv_flags {
	PRV_EMITDUP     = 1<<0, /* Emit duplicates (no error, emit) */
	PRV_SKIPDUP     = 1<<1, /* Skip duplicates (no error, no emit) */
	PRV_NEXT        = 1<<2, /* Add one to the channel value */
	PRV_ZERO        = 1<<3, /* Value 0 is allowed */
	PRV_SKIPDUPNULL = 1<<4, /* Skip duplicates if the value is null (no error, no emit) */
};

struct prv_chan {
	struct prv *prv;
	struct chan *chan;
	long id;
	long row_base1;
	long type;
	long flags;
	int last_value_set;
	struct value last_value;
	UT_hash_handle hh; /* Indexed by chan->name */
};

struct prv {
	FILE *file;
	int64_t time;
	long nrows;
	struct prv_chan *channels;
};

USE_RET int prv_open(struct prv *prv, long nrows, const char *path);
USE_RET int prv_open_file(struct prv *prv, long nrows, FILE *file);
USE_RET int prv_register(struct prv *prv, long row, long type, struct bay *bay, struct chan *chan, long flags);
USE_RET int prv_advance(struct prv *prv, int64_t time);
USE_RET int prv_close(struct prv *prv);

#endif /* PRV_H */
