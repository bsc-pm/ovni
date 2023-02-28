/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef SORT_H
#define SORT_H

#include <stdint.h>
#include "bay.h"

struct sort;

struct sort_input {
	int64_t index;
	struct chan *chan;
	struct sort *sort;
};

struct sort {
	int64_t n;
	struct sort_input *inputs;
	struct chan **outputs;
	int64_t *values;
	int64_t *sorted;
	struct bay *bay;
};

USE_RET int sort_init(struct sort *sort, struct bay *bay, int64_t n);
USE_RET int sort_set_input(struct sort *sort, int64_t index, struct chan *input);
USE_RET int sort_set_output(struct sort *sort, int64_t index, struct chan *output);
USE_RET int sort_register(struct sort *sort, struct bay *bay);

#endif /* SORT_H */
