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

	dbg("selecting mux input %p\n", (void *) input);

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

	/* TODO: We may need to cache the last read value from select to avoid
	 * problems reading the select channel too soon */
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
	if (input == NULL || input->chan != in_chan)
		return 0;

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

	memset(mux, 0, sizeof(struct mux_input));
	mux->select = select;
	mux->output = output;

	if (select_func == NULL)
		select_func = default_select;

	mux->select_func = select_func;
	mux->bay = bay;

	if (bay_add_cb(bay, select, cb_select, mux) != 0) {
		err("mux_init: bay_add_cb failed\n");
		return -1;
	}

	return 0;
}

struct mux_input *
mux_find_input(struct mux *mux, struct value value)
{
	struct mux_input *input = NULL;
	/* Only int64 due to garbage */
	if (value.type != VALUE_INT64)
		die("bad value type\n");

	HASH_FIND(hh, mux->input, &value.i, sizeof(value.i), input);
	return input;
}

int
mux_add_input(struct mux *mux, struct value key, struct chan *chan)
{
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

	HASH_ADD_KEYPTR(hh, mux->input, &input->key.i, sizeof(input->key.i), input);

	if (bay_add_cb(mux->bay, chan, cb_input, mux) != 0) {
		err("mux_add_input: bay_add_cb failed\n");
		return -1;
	}

	return 0;
}
