/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "sort.h"
#include <stdlib.h>
#include <string.h>
#include "bay.h"
#include "chan.h"
#include "value.h"

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

/** Replaces the value old in the array arr by new, while keeping the
 * array arr sorted.
 *
 * Preconditions:
 *  - arr is sorted
 *  - old is in arr
 *  - old != new
 */
void
sort_replace(int64_t *arr, int64_t n, int64_t old, int64_t new)
{
	if (unlikely(old == new))
		die("old == new");

	/* Remove the old first then insert the new */
	if (old < new) {
		int64_t i = 0;

		/* Quick jump to middle if less than old */
		int64_t m = n / 2;
		if (arr[m] < old)
			i = m;

		/* Skip content until old, no need to copy */
		for (; arr[i] < old; i++)
			;

		/* Copy middle section replacing old */
		for (; i < n - 1 && arr[i + 1] <= new; i++)
			arr[i] = arr[i + 1];

		/* Place new */
		arr[i] = new;
	} else { /* new < old */
		int64_t i = 0;

		/* Find old, must be found */
		for (; arr[i] < old; i++)
			;

		/* Shift right to replace old */
		for (; i > 0 && arr[i - 1] > new; i--)
			arr[i] = arr[i - 1];

		/* Invariant: Either i == 0 or arr[i] <= new
		 * and the element arr[i] must be gone */

		/* Place new */
		arr[i] = new;
	}
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

	int64_t new = 0;
	if (cur.type == VALUE_INT64)
		new = cur.i;

	int64_t index = input->index;
	int64_t old = sort->values[index];

	/* Nothing to do if no change */
	if (old == new)
		return 0;

	dbg("sort input %s changed", in_chan->name);

	/* Otherwise recompute the outputs */
	sort->values[index] = new;

	if (likely(sort->copied)) {
		sort_replace(sort->sorted, sort->n, old, new);
	} else {
		memcpy(sort->sorted, sort->values, (size_t) sort->n * sizeof(int64_t));
		qsort(sort->sorted, (size_t) sort->n, sizeof(int64_t), cmp_int64);
		sort->copied = 1;
	}

	for (int64_t i = 0; i < sort->n; i++) {
		struct value val = value_int64(sort->sorted[i]);
		struct value last;
		if (chan_read(&sort->outputs[i], &last) != 0) {
			err("chan_read failed");
			return -1;
		}

		if (value_is_equal(&last, &val))
			continue;

		dbg("writting value %s into channel %s",
				value_str(val),
				sort->outputs[i].name);

		if (chan_set(&sort->outputs[i], val) != 0) {
			err("chan_set failed");
			return -1;
		}
	}

	return 0;
}

int
sort_init(struct sort *sort, struct bay *bay, int64_t n, const char *name)
{
	memset(sort, 0, sizeof(struct sort));
	sort->bay = bay;
	sort->n = n;
	sort->inputs = calloc((size_t) n, sizeof(struct sort_input));
	if (sort->inputs == NULL) {
		err("calloc failed:");
		return -1;
	}
	sort->outputs = calloc((size_t) n, sizeof(struct chan));
	if (sort->outputs == NULL) {
		err("calloc failed:");
		return -1;
	}
	sort->values = calloc((size_t) n, sizeof(int64_t));
	if (sort->values == NULL) {
		err("calloc failed:");
		return -1;
	}
	sort->sorted = calloc((size_t) n, sizeof(int64_t));
	if (sort->sorted == NULL) {
		err("calloc failed:");
		return -1;
	}

	/* Init and register outputs */
	for (int64_t i = 0; i < n; i++) {
		struct chan *out = &sort->outputs[i];
		chan_init(out, CHAN_SINGLE, "%s.out%"PRIi64, name, i);

		/* The sort module may write multiple times to the same
		 * channel if we update more than one input. */
		chan_prop_set(out, CHAN_DIRTY_WRITE, 1);

		/* No duplicate checks are done by sort module, so we
		 * simply allow them */
		chan_prop_set(out, CHAN_ALLOW_DUP, 1);

		if (bay_register(bay, out) != 0) {
			err("bay_register out%"PRIi64" failed", i);
			return -1;
		}
	}

	return 0;
}

int
sort_set_input(struct sort *sort, int64_t index, struct chan *chan)
{
	struct sort_input *input = &sort->inputs[index];

	if (input->chan != NULL) {
		err("input %"PRIi64" already has a channel", index);
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

struct chan *
sort_get_output(struct sort *sort, int64_t index)
{
	return &sort->outputs[index];
}
