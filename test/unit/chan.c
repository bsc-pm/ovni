#include "emu/chan.h"
#include "common.h"

static void
check_single(void)
{
	struct chan chan;
	struct value one = { .type = VALUE_INT64, .i = 1 };
	struct value two = { .type = VALUE_INT64, .i = 2 };
	//struct value nil = { .type = VALUE_NULL, .i = 0 };

	chan_init(&chan, CHAN_SINGLE, "testchan");

	/* Ensure we cannot push as stack */
	if (chan_push(&chan, one) == 0)
		die("chan_push didn't fail\n");

	/* Now we should be able to write with set */
	if (chan_set(&chan, one) != 0)
		die("chan_set failed\n");

	/* Now is dirty, it shouldn't allow another set */
	if (chan_set(&chan, two) == 0)
		die("chan_set didn't fail\n");

	struct value value;

	if (chan_read(&chan, &value) != 0)
		die("chan_read failed\n");

	if (!value_is_equal(&value, &one))
		die("chan_read returned unexpected value\n");
}

//static void
//check_stack(void)
//{
//	struct chan chan;
//
//	chan_init(&chan, CHAN_STACK);
//
//	/* Ensure we cannot set as single */
//	if (chan_set(&chan, 1) == 0)
//		die("chan_set didn't fail\n");
//
//	/* Channels are closed after init */
//	if (chan_push(&chan, 1) != 0)
//		die("chan_push failed\n");
//
//	/* Now is closed, it shouldn't allow another value */
//	if (chan_push(&chan, 2) == 0)
//		die("chan_push didn't fail\n");
//
//	struct chan_value value = { 0 };
//
//	if (chan_flush(&chan, &value) != 0)
//		die("chan_flush failed\n");
//
//	if (!value.ok || value.i != 1)
//		die("chan_flush returned unexpected value\n");
//
//	/* Now it should allow to push another value */
//	if (chan_push(&chan, 2) != 0)
//		die("chan_push failed\n");
//
//	if (chan_flush(&chan, &value) != 0)
//		die("chan_flush failed\n");
//
//	if (!value.ok || value.i != 2)
//		die("chan_flush returned unexpected value\n");
//
//	/* Now pop the values */
//	if (chan_pop(&chan, 2) != 0)
//		die("chan_pop failed\n");
//
//	if (chan_flush(&chan, &value) != 0)
//		die("chan_flush failed\n");
//
//	if (!value.ok || value.i != 1)
//		die("chan_flush returned unexpected value\n");
//
//	if (chan_pop(&chan, 1) != 0)
//		die("chan_pop failed\n");
//
//	/* Now the stack should be empty */
//
//	if (chan_pop(&chan, 1) == 0)
//		die("chan_pop didn't fail\n");
//
//}

int main(void)
{
	check_single();
	//check_stack();
	return 0;
}
