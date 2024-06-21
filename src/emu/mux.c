/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "mux.h"
#include <stdlib.h>
#include <string.h>
#include "bay.h"
#include "chan.h"

static int
default_select(struct mux *mux,
		struct value key,
		struct mux_input **pinput)
{
	if (key.type == VALUE_NULL) {
		*pinput = NULL;
		return 0;
	}

	if (key.type != VALUE_INT64) {
		err("only null and int64 values supported");
		return -1;
	}

	int64_t index = key.i;

	if (index < 0 || index >= mux->ninputs) {
		err("index out of bounds %"PRIi64, index);
		return -1;
	}

	struct mux_input *input = &mux->inputs[index];

	*pinput = input;

	return 0;
}

static int
select_input(struct mux *mux,
		struct value key,
		struct mux_input **input)
{
	if (mux->select_func == NULL) {
		if (default_select(mux, key, input) != 0) {
			err("default_select failed");
			return -1;
		}
	} else if (mux->select_func(mux, key, input) != 0) {
		err("select_func failed");
		return -1;
	}

	return 0;
}

/** Called when the select channel changes its value */
static int
cb_select(struct chan *sel_chan, void *ptr)
{
	struct mux *mux = ptr;

	dbg("selecting input for output chan chan=%s", mux->output->name);

	struct value sel_value;
	if (chan_read(sel_chan, &sel_value) != 0) {
		err("chan_read(select) failed");
		return -1;
	}

	/* Clear previous selected input */
	if (mux->selected >= 0) {
		struct mux_input *old_input = &mux->inputs[mux->selected];
		bay_disable_cb(old_input->cb);
		old_input->selected = 0;
		mux->selected = -1;
	}

	dbg("select channel got value %s",
			value_str(sel_value));

	struct mux_input *input = NULL;
	if (select_input(mux, sel_value, &input) != 0) {
		err("select_input failed");
		return -1;
	}

	if (input) {
		bay_enable_cb(input->cb);
		input->selected = 1;
		mux->selected = input->index;
		dbg("mux selects input key=%s chan=%s",
				value_str(sel_value), input->chan->name);
	}

	struct value out_value = mux->def;
	if (input != NULL && chan_read(input->chan, &out_value) != 0) {
		err("chan_read() failed");
		return -1;
	}

	dbg("setting output chan %s to %s",
			mux->output->name, value_str(out_value));

	if (chan_set(mux->output, out_value) != 0) {
		err("chan_set() failed");
		return -1;
	}

	return 0;
}

/** Called when the input channel changes its value and is selected */
static int
cb_input(struct chan *in_chan, void *ptr)
{
	struct mux_input *input = ptr;

	dbg("selected mux input %s changed", in_chan->name);

	struct value out_value;

	if (chan_read(in_chan, &out_value) != 0) {
		err("chan_read() failed");
		return -1;
	}

	dbg("setting output chan %s to value %s",
			input->output->name, value_str(out_value));

	if (chan_set(input->output, out_value) != 0) {
		err("chan_set() failed");
		return -1;
	}

	return 0;
}

int
mux_init(struct mux *mux,
		struct bay *bay,
		struct chan *select,
		struct chan *output,
		mux_select_func_t select_func,
		int64_t ninputs)
{
	if (chan_get_type(output) != CHAN_SINGLE) {
		err("output channel must be of type single");
		return -1;
	}

	if (select == output) {
		err("cannot use same channel as select and output");
		return -1;
	}

	/* Ensure both channels are registered */
	if (bay_find(bay, select->name) == NULL) {
		err("select channel %s not registered in bay", select->name);
		return -1;
	}
	if (bay_find(bay, output->name) == NULL) {
		err("output channel %s not registered in bay", output->name);
		return -1;
	}

	/* The output channel must accept multiple writes in the same
	 * propagation phase while the channel is dirty, as we may write to the
	 * input and select channel at the same time. */
	chan_prop_set(output, CHAN_DIRTY_WRITE, 1);

	/* Similarly, we may switch to an input channel that has the same value
	 * as the last output value, so we allow duplicates too */
	chan_prop_set(output, CHAN_ALLOW_DUP, 1);

	memset(mux, 0, sizeof(struct mux));
	mux->select = select;
	mux->output = output;
	mux->ninputs = ninputs;
	mux->inputs = calloc(ninputs, sizeof(struct mux_input));
	mux->def = value_null();

	if (mux->inputs == NULL) {
		err("calloc failed:");
		return -1;
	}

	mux->select_func = select_func;
	mux->bay = bay;

	/* Select always enabled */
	if (bay_add_cb(bay, BAY_CB_DIRTY, select, cb_select, mux, 1) == NULL) {
		err("bay_add_cb failed");
		return -1;
	}

	return 0;
}

struct mux_input *
mux_get_input(struct mux *mux, int64_t index)
{
	return &mux->inputs[index];
}

int
mux_set_input(struct mux *mux, int64_t index, struct chan *chan)
{
	if (chan == mux->output) {
		err("cannot use same input channel as output");
		return -1;
	}

	struct mux_input *input = &mux->inputs[index];

	if (input->chan != NULL) {
		err("input %"PRIi64" already has a channel", index);
		return -1;
	}

	input->index = index;
	input->chan = chan;
	input->output = mux->output;

	/* Inputs disabled until selected */
	input->cb = bay_add_cb(mux->bay, BAY_CB_DIRTY, chan, cb_input, input, 0);
	if (input->cb == NULL) {
		err("bay_add_cb failed");
		return -1;
	}

	return 0;
}

void
mux_set_default(struct mux *mux, struct value def)
{
	mux->def = def;
}
