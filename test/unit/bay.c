#include "emu/bay.h"
#include "common.h"

static int
callback(struct chan *chan, void *ptr)
{
	struct value value;
	if (chan_read(chan, &value) != 0)
		die("callback: chan_read failed\n");

	if (value.type != VALUE_INT64)
		die("callback: unexpected value type\n");

	int64_t *ival = ptr;
	*ival = value.i;

	return 0;
}

int main(void)
{
	struct bay bay;
	bay_init(&bay);

	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");

	bay_register(&bay, &chan);

	int64_t data = 0;
	bay_add_cb(&bay, &chan, callback, &data);

	if (data != 0)
		die("data changed after bay_chan_append_cb\n");

	struct value one = value_int64(1);
	if (chan_set(&chan, one) != 0)
		die("chan_set failed\n");

	if (data != 0)
		die("data changed after chan_set\n");

	/* Now the callback should modify 'data' */
	if (bay_propagate(&bay) != 0)
		die("bay_propagate failed\n");

	if (data != 1)
		die("data didn't change after bay_propagate\n");

	return 0;
}
