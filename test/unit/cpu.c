#include "emu/bay.h"
#include "common.h"

static void
test_remove(struct bay *bay)
{
	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "removeme");

	if (bay_register(bay, &chan) != 0)
		die("bay_register failed\n");

	if (bay_find(bay, chan.name) == NULL)
		die("bay_find failed\n");

	if (bay_remove(bay, &chan) != 0)
		die("bay_remove failed\n");

	if (bay_find(bay, chan.name) != NULL)
		die("bay_find didn't fail\n");
}

static void
test_duplicate(struct bay *bay)
{
	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "dup");

	if (bay_register(bay, &chan) != 0)
		die("bay_register failed\n");

	if (bay_register(bay, &chan) == 0)
		die("bay_register didn't fail\n");

	if (bay_remove(bay, &chan) != 0)
		die("bay_remove failed\n");
}

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

static void
test_callback(struct bay *bay)
{
	struct chan chan;
	chan_init(&chan, CHAN_SINGLE, "testchan");

	if (bay_register(bay, &chan) != 0)
		die("bay_register failed\n");

	int64_t data = 0;
	if (bay_add_cb(bay, BAY_CB_DIRTY, &chan, callback, &data) != 0)
		die("bay_add_cb failed\n");

	if (data != 0)
		die("data changed after bay_chan_append_cb\n");

	if (chan_set(&chan, value_int64(1)) != 0)
		die("chan_set failed\n");

	if (data != 0)
		die("data changed after chan_set\n");

	/* Now the callback should modify 'data' */
	if (bay_propagate(bay) != 0)
		die("bay_propagate failed\n");

	if (data != 1)
		die("data didn't change after bay_propagate\n");

	if (bay_remove(bay, &chan) != 0)
		die("bay_remove failed\n");
}

int main(void)
{
	struct bay bay;
	bay_init(&bay);

	test_remove(&bay);
	test_duplicate(&bay);
	test_callback(&bay);

	return 0;
}
