#include "emu/mux.h"
#include "common.h"

#define N 4

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
	bay_register(&bay, &output);
	bay_register(&bay, &select);
	for (int i = 0; i < N; i++) {
		bay_register(&bay, &inputs[i]);
	}

	struct mux mux;
	mux_init(&mux, &bay, &select, &output, NULL);

	for (int i = 0; i < N; i++)
		mux_add_input(&mux, value_int64(i), &inputs[i]);

	/* Write something to the input channels */
	for (int i = 0; i < N; i++) {
		if (chan_set(&inputs[i], value_int64(1000 + i)) != 0)
			die("chan_set failed\n");
	}

	/* Propagate values and call the callbacks */
	if (bay_propagate(&bay) != 0)
		die("bay_propagate failed\n");


	/* Change select channel */
	if (chan_set(&select, value_int64(2)) != 0)
		die("chan_set failed\n");

	/* Propagate values and call the callbacks */
	if (bay_propagate(&bay) != 0)
		die("bay_propagate failed\n");

	struct value out_value = value_null();
	if (chan_read(&output, &out_value) != 0)
		die("chan_read() failed for output channel\n");

	struct value expected_value = value_int64(1002);
	if (!value_is_equal(&out_value, &expected_value)) {
		char buf1[128];
		char buf2[128];
		die("unexpected value found %s in output (expected %s)\n",
				value_str(out_value, buf1),
				value_str(expected_value, buf2));
	}

	err("OK\n");

	return 0;
}
