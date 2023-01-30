#define ENABLE_DEBUG

#include "mux.h"

static int
default_select(struct mux *mux,
		struct value key,
		struct mux_input **input)
{
	if (value_is_null(key)) {
		*input = NULL;
		return 0;
	}

	struct mux_input *tmp = mux_find_input(mux, key);

	if (tmp == NULL) {
		char buf[128];
		err("default_select: cannot find input with key %s\n",
				value_str(key, buf));
		return -1;
	}

	*input = tmp;

	return 0;
}

static int
select_input(struct mux *mux,
		struct value key,
		struct mux_input **input)
{
	if (mux->select_func == NULL)
		die("select_input: select_func is NULL\n");

	if (mux->select_func(mux, key, input) != 0) {
		err("select_input: select_func failed\n");
		return -1;
	}

	return 0;
}

/** Called when the select channel changes its value */
static int
cb_select(struct chan *sel_chan, void *ptr)
{
	struct mux *mux = ptr;

	struct value sel_value;
	if (chan_read(sel_chan, &sel_value) != 0) {
		err("cb_select: chan_read(select) failed\n");
		return -1;
	}

	char buf[128];
	dbg("select channel got value %s\n",
			value_str(sel_value, buf));

	struct mux_input *input = NULL;
	if (select_input(mux, sel_value, &input) != 0) {
		err("cb_select: select_input failed\n");
		return -1;
	}

	if (input) {
		dbg("mux selects input key=%s chan=%s\n",
				value_str(sel_value, buf), input->chan->name);
	}

	/* Set to null by default */
	struct value out_value = value_null();
	if (input != NULL && chan_read(input->chan, &out_value) != 0) {
		err("cb_select: chan_read() failed\n");
		return -1;
	}

	if (chan_set(mux->output, out_value) != 0) {
		err("cb_select: chan_set() failed\n");
		return -1;
	}

	return 0;
}

/** Called when the input channel changes its value */
static int
cb_input(struct chan *in_chan, void *ptr)
{
	struct mux *mux = ptr;

	struct value sel_value;
	if (chan_read(mux->select, &sel_value) != 0) {
		err("cb_input: chan_read(select) failed\n");
		return -1;
	}

	struct mux_input *input = NULL;
	if (select_input(mux, sel_value, &input) != 0) {
		err("cb_input: select_input failed\n");
		return -1;
	}

	/* Nothing to do, the input is not selected */
	if (input == NULL || input->chan != in_chan) {
		dbg("mux: input channel %s changed but not selected\n",
				in_chan->name);
		return 0;
	}

	dbg("selected mux input %s changed\n", in_chan->name);

	/* Set to null by default */
	struct value out_value = value_null();

	if (chan_read(in_chan, &out_value) != 0) {
		err("cb_input: chan_read() failed\n");
		return -1;
	}

	if (chan_set(mux->output, out_value) != 0) {
		err("cb_input: chan_set() failed\n");
		return -1;
	}

	return 0;
}

int
mux_init(struct mux *mux,
		struct bay *bay,
		struct chan *select,
		struct chan *output,
		mux_select_func_t select_func)
{
	if (chan_get_type(output) != CHAN_SINGLE) {
		err("mux_init: output channel must be of type single\n");
		return -1;
	}

	if (select == output) {
		err("mux_init: cannot use same channel as select and output\n");
		return -1;
	}

	/* Ensure both channels are registered */
	if (bay_find(bay, select->name) == NULL) {
		err("mux_init: select channel %s not registered in bay\n",
				select->name);
		return -1;
	}
	if (bay_find(bay, output->name) == NULL) {
		err("mux_init: output channel %s not registered in bay\n",
				output->name);
		return -1;
	}

	/* The output channel must accept multiple writes in the same
	 * propagation phase while the channel is dirty, as we may write to the
	 * input and select channel at the same time. */
	chan_prop_set(output, CHAN_DIRTY_WRITE, 1);

	/* Similarly, we may switch to an input channel that has the same value
	 * as the last output value, so we allow duplicates too */
	chan_prop_set(output, CHAN_DUPLICATES, 1);

	memset(mux, 0, sizeof(struct mux_input));
	mux->select = select;
	mux->output = output;

	if (select_func == NULL)
		select_func = default_select;

	mux->select_func = select_func;
	mux->bay = bay;

	if (bay_add_cb(bay, BAY_CB_DIRTY, select, cb_select, mux) != 0) {
		err("mux_init: bay_add_cb failed\n");
		return -1;
	}

	return 0;
}

struct mux_input *
mux_find_input(struct mux *mux, struct value value)
{
	struct mux_input *input = NULL;

	HASH_FIND(hh, mux->input, &value, sizeof(value), input);
	return input;
}

int
mux_add_input(struct mux *mux, struct value key, struct chan *chan)
{
	if (chan == mux->output) {
		err("mux_init: cannot use same input channel as output\n");
		return -1;
	}

	if (chan == mux->select) {
		err("mux_init: cannot use same input channel as select\n");
		return -1;
	}

	if (mux_find_input(mux, key) != NULL) {
		char buf[128];
		err("mux_add_input: input key %s already in mux\n",
				value_str(key, buf));
		return -1;
	}

	struct mux_input *input = calloc(1, sizeof(struct mux_input));

	if (input == NULL) {
		err("mux_add_input: calloc failed\n");
		return -1;
	}

	input->key = key;
	input->chan = chan;

	HASH_ADD_KEYPTR(hh, mux->input, &input->key, sizeof(input->key), input);

	if (bay_add_cb(mux->bay, BAY_CB_DIRTY, chan, cb_input, mux) != 0) {
		err("mux_add_input: bay_add_cb failed\n");
		return -1;
	}

	mux->ninputs++;

	return 0;
}
