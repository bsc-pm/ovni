/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PRV_H
#define PRV_H

#include "chan.h"
#include "bay.h"
#include "uthash.h"
#include <stdio.h>

struct prv;

enum prv_flags {
	PRV_DUP = 1,
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

int prv_open(struct prv *prv, long nrows, const char *path);
int prv_open_file(struct prv *prv, long nrows, FILE *file);
int prv_register(struct prv *prv, long row, long type, struct bay *bay, struct chan *chan, long flags);
int prv_advance(struct prv *prv, int64_t time);
int prv_close(struct prv *prv);

#endif /* PRV_H */
