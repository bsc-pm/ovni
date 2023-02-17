#include "emu/mux.h"
#include "common.h"

#define N 10

//static int
//select_active_thread(struct mux *mux,
//		struct value *value,
//		struct mux_input **input)
//{
//	if (value->type == VALUE_NULL) {
//		*input = NULL;
//		return 0;
//	}
//
//	if (value->type != VALUE_INT64) {
//		err("expecting NULL or INT64 channel value\n");
//		return -1;
//	}
//
//	enum thread_state state = (enum thread_state) value->i;
//
//	if (mux->ninputs != 1) {
//		err("expecting NULL or INT64 channel value\n");
//		return -1;
//	}
//
//	switch (state) {
//		case TH_ST_RUNNING:
//		case TH_ST_COOLING:
//		case TH_ST_WARMING:
//			*input = only_input;
//			break;
//		case TH_ST_PAUSED:
//			*input = NULL;
//			break;
//	}
//
//	return 0;
//}

static void
check_output(struct mux *mux, struct value expected)
{
	struct value out_value = value_null();
	if (chan_read(mux->output, &out_value) != 0)
		die("chan_read() failed for output channel\n");

	if (!value_is_equal(&out_value, &expected)) {
		char buf1[128];
		char buf2[128];
		die("unexpected value found %s in output (expected %s)\n",
				value_str(out_value, buf1),
				value_str(expected, buf2));
	}

	err("----- output ok -----\n");
}

static void
test_select(struct mux *mux, int key)
{
	if (chan_set(mux->select, value_int64(key)) != 0)
		die("chan_set failed\n");

	if (bay_propagate(mux->bay) != 0)
		die("bay_propagate failed\n");

	check_output(mux, value_int64(1000 + key));
}

static void
test_input(struct mux *mux, int key)
{
	/* Set the select channel to the selected key */
	test_select(mux, key);

	int new_value = 2000 + key;

	/* Then change that channel */
	struct mux_input *mi = mux_get_input(mux, key);
	if (mi == NULL)
		die("mux_get_input failed to locate input %d\n", key);

	if (chan_set(mi->chan, value_int64(new_value)) != 0)
		die("chan_set failed\n");

	if (bay_propagate(mux->bay) != 0)
		die("bay_propagate failed\n");

	check_output(mux, value_int64(new_value));
}

static void
test_select_and_input(struct mux *mux, int key)
{
	/* Set the select channel to the selected key, but don't
	 * propagate the changes yet */
	if (chan_set(mux->select, value_int64(key)) != 0)
		die("chan_set failed\n");

	int new_value = 2000 + key;

	/* Also change that channel */
	struct mux_input *mi = mux_get_input(mux, key);
	if (mi == NULL)
		die("mux_get_input failed to locate input %d\n", key);

	if (chan_set(mi->chan, value_int64(new_value)) != 0)
		die("chan_set failed\n");

	/* Write twice to the output */
	if (bay_propagate(mux->bay) != 0)
		die("bay_propagate failed\n");

	check_output(mux, value_int64(new_value));
}

static void
test_input_and_select(struct mux *mux, int key)
{
	int new_value = 2000 + key;

	/* First change the input */
	struct mux_input *mi = mux_get_input(mux, key);
	if (mi == NULL)
		die("mux_get_input failed to locate input %d\n", key);

	if (chan_set(mi->chan, value_int64(new_value)) != 0)
		die("chan_set failed\n");

	/* Then change select */
	if (chan_set(mux->select, value_int64(key)) != 0)
		die("chan_set failed\n");

	/* Write twice to the output */
	if (bay_propagate(mux->bay) != 0)
		die("bay_propagate failed\n");

	check_output(mux, value_int64(new_value));
}

static void
test_mid_propagate(struct mux *mux, int key)
{
	int new_value = 2000 + key;

	struct mux_input *mi = mux_get_input(mux, key);
	if (mi == NULL)
		die("mux_get_input failed to locate input %d\n", key);

	if (chan_set(mi->chan, value_int64(new_value)) != 0)
		die("chan_set failed\n");

	if (bay_propagate(mux->bay) != 0)
		die("bay_propagate failed\n");

	if (chan_set(mux->select, value_int64(key)) != 0)
		die("chan_set failed\n");

	if (bay_propagate(mux->bay) != 0)
		die("bay_propagate failed\n");

	check_output(mux, value_int64(new_value));
}

static void
test_duplicate_output(struct mux *mux, int key1, int key2)
{
	int new_value = 2000 + key1;

	struct mux_input *in1 = mux_get_input(mux, key1);
	if (in1 == NULL)
		die("mux_get_input failed to locate input1 %d\n", key1);

	struct mux_input *in2 = mux_get_input(mux, key2);
	if (in2 == NULL)
		die("mux_get_input failed to locate input2 %d\n", key2);

	if (chan_set(in1->chan, value_int64(new_value)) != 0)
		die("chan_set failed\n");

	if (chan_set(in2->chan, value_int64(new_value)) != 0)
		die("chan_set failed\n");

	/* Select input 1 */
	if (chan_set(mux->select, value_int64(key1)) != 0)
		die("chan_set failed\n");

	if (bay_propagate(mux->bay) != 0)
		die("bay_propagate failed\n");

	check_output(mux, value_int64(new_value));

	/* Now switch to input 2, which has the same value */
	if (chan_set(mux->select, value_int64(key2)) != 0)
		die("chan_set failed\n");

	if (bay_propagate(mux->bay) != 0)
		die("bay_propagate failed\n");

	check_output(mux, value_int64(new_value));
}


int
main(void)
{
	struct bay bay;
	bay_init(&bay);

	struct chan inputs[N];
	struct chan output;
	struct chan select;

	chan_init(&output, CHAN_SINGLE, "output");
	chan_init(&select, CHAN_SINGLE, "select");

	for (int i = 0; i < N; i++) {
		char buf[MAX_CHAN_NAME];
		sprintf(buf, "input.%d", i);
		chan_init(&inputs[i], CHAN_SINGLE, buf);
	}

	/* Register all channels in the bay */
	if (bay_register(&bay, &select) != 0)
		die("bay_register failed\n");

	for (int i = 0; i < N; i++) {
		if(bay_register(&bay, &inputs[i]) != 0)
			die("bay_register failed\n");
	}

	struct mux mux;
	/* Attempt to init the mux without registering the output */
	if (mux_init(&mux, &bay, &select, &output, NULL, N) == 0)
		die("mux_init didn't fail\n");

	if (bay_register(&bay, &output) != 0)
		die("bay_register failed\n");

	if (mux_init(&mux, &bay, &select, &output, NULL, N) != 0)
		die("mux_init failed\n");

	for (int i = 0; i < N; i++) {
		if (mux_set_input(&mux, i, &inputs[i]) != 0)
			die("mux_add_input failed\n");
	}

	/* Write something to the input channels */
	for (int i = 0; i < N; i++) {
		if (chan_set(&inputs[i], value_int64(1000 + i)) != 0)
			die("chan_set failed\n");
	}

	/* Propagate values and call the callbacks */
	if (bay_propagate(&bay) != 0)
		die("bay_propagate failed\n");

	test_select(&mux, 1);
	test_input(&mux, 2);
	test_select_and_input(&mux, 3);
	test_input_and_select(&mux, 4);
	test_mid_propagate(&mux, 5);
	test_duplicate_output(&mux, 6, 7);

	err("OK\n");

	return 0;
}
