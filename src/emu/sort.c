/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

//#define ENABLE_DEBUG

#include "sort.h"

static int
cmp_int64(const void *a, const void *b)
{
	int64_t aa = *(const int64_t *) a;
	int64_t bb = *(const int64_t *) b;

	if (aa < bb)
		return -1;
	else if (aa > bb)
		return +1;
	else
		return 0;
}

/** Called when an input channel changes its value */
static int
sort_cb_input(struct chan *in_chan, void *ptr)
{
	struct sort_input *input = ptr;
	struct sort *sort = input->sort;
	struct value cur;

	if (chan_read(in_chan, &cur) != 0) {
		err("chan_read() failed\n");
		return -1;
	}

	int64_t vcur = 0;
	if (cur.type == VALUE_INT64)
		vcur = cur.i;

	int64_t index = input->index;
	int64_t last = sort->values[index];

	/* Nothing to do if no change */
	if (last == vcur)
		return 0;

	dbg("sort input %s changed", in_chan->name);

	/* Otherwise recompute the outputs */
	sort->values[index] = vcur;
	memcpy(sort->sorted, sort->values, sort->n * sizeof(int64_t));
	qsort(sort->sorted, sort->n, sizeof(int64_t), cmp_int64);

	for (int64_t i = 0; i < sort->n; i++) {
		struct value val = value_int64(sort->sorted[i]);
		if (chan_set(sort->outputs[i], val) != 0) {
			err("chan_set failed");
			return -1;
		}
	}

	return 0;
}

int
sort_init(struct sort *sort, struct bay *bay, int64_t n)
{
	memset(sort, 0, sizeof(struct sort));
	sort->bay = bay;
	sort->n = n;
	sort->inputs = calloc(n, sizeof(struct sort_input));
	if (sort->inputs == NULL) {
		err("calloc failed:");
		return -1;
	}
	sort->outputs = calloc(n, sizeof(struct chan *));
	if (sort->outputs == NULL) {
		err("calloc failed:");
		return -1;
	}
	sort->values = calloc(n, sizeof(int64_t));
	if (sort->values == NULL) {
		err("calloc failed:");
		return -1;
	}
	sort->sorted = calloc(n, sizeof(int64_t));
	if (sort->sorted == NULL) {
		err("calloc failed:");
		return -1;
	}

	return 0;
}

int
sort_set_input(struct sort *sort, int64_t index, struct chan *chan)
{
	struct sort_input *input = &sort->inputs[index];

	if (input->chan != NULL) {
		err("input %d already has a channel", index);
		return -1;
	}

	input->chan = chan;
	input->index = index;
	input->sort = sort;

	if (bay_add_cb(sort->bay, BAY_CB_DIRTY, chan, sort_cb_input, input, 1) == NULL) {
		err("bay_add_cb failed");
		return -1;
	}

	return 0;
}

int
sort_set_output(struct sort *sort, int64_t index, struct chan *chan)
{
	if (sort->outputs[index] != NULL) {
		err("output %d already has a channel", index);
		return -1;
	}

	sort->outputs[index] = chan;

	/* The sort module may write multiple times to the same channel if we
	 * update more than one input. */
	chan_prop_set(chan, CHAN_DIRTY_WRITE, 1);

	/* No duplicate checks are done by sort module, so we simply allow them */
	chan_prop_set(chan, CHAN_DUPLICATES, 1);

	return 0;
}
