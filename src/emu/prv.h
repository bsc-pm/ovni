/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PRV_H
#define PRV_H

#include "chan.h"
#include "bay.h"
#include "uthash.h"
#include <stdio.h>

struct prv;

struct prv_chan {
	struct prv *prv;
	struct chan *chan;
	long row_base1;
	long type;
	int last_value_set;
	struct value last_value;
	UT_hash_handle hh; /* Indexed by chan->name */
};

struct prv {
	FILE *file;
	struct bay *bay;
	int64_t time;
	long nrows;
	struct prv_chan *channels;
};

int prv_open(struct prv *prv, struct bay *bay, long nrows, const char *path);
int prv_open_file(struct prv *prv, struct bay *bay, long nrows, FILE *file);
int prv_register(struct prv *prv, long row, long type, struct chan *c);
int prv_advance(struct prv *prv, int64_t time);
void prv_close(struct prv *prv);

#endif /* PRV_H */
