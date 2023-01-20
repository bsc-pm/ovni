/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef CLKOFF_H
#define CLKOFF_H

#include "uthash.h"

#include <stdio.h>
#include <linux/limits.h>

struct clkoff_entry {
	int64_t index;
	char name[PATH_MAX];
	double median;
	double mean;
	double stdev;
	UT_hash_handle hh;
};

struct clkoff {
	int nentries;
	struct clkoff_entry *entries;
	struct clkoff_entry **index;
};

int clkoff_init(struct clkoff *table, FILE *file);
int clkoff_count(struct clkoff *table);
struct clkoff_entry *clkoff_get(struct clkoff *table, int i);

#endif /* CLKOFF_H */
