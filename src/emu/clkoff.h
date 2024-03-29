/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef CLKOFF_H
#define CLKOFF_H

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include "common.h"
#include "uthash.h"

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

        void clkoff_init(struct clkoff *table);
USE_RET int clkoff_load(struct clkoff *table, FILE *file);
USE_RET int clkoff_count(struct clkoff *table);
USE_RET struct clkoff_entry *clkoff_get(struct clkoff *table, int i);

#endif /* CLKOFF_H */
