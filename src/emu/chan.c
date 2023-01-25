/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "chan.h"
#include "common.h"
#include <string.h>
#include <stdarg.h>

void
chan_init(struct chan *chan, enum chan_type type, const char *fmt, ...)
{
	memset(chan, 0, sizeof(struct chan));

	va_list ap;
	va_start(ap, fmt);

	int n = ARRAYLEN(chan->name);
	int ret = vsnprintf(chan->name, n, fmt, ap);
	if (ret >= n)
		die("channel name too long\n");
	va_end(ap);

	chan->type = type;
}

void
chan_set_dirty_cb(struct chan *chan, chan_cb_t func, void *arg)
{
	chan->dirty_cb = func;
	chan->dirty_arg = arg;
}

enum chan_type
chan_get_type(struct chan *chan)
{
	return chan->type;
}

static int
set_dirty(struct chan *chan)
{
	if (chan->is_dirty) {
		/* Already dirty and allowed, no need to do anything */
		if (chan->prop[CHAN_DIRTY_WRITE])
			return 0;

		err("channel %s already dirty\n", chan->name);
		return -1;
	}

	chan->is_dirty = 1;

	if (chan->dirty_cb != NULL) {
		if (chan->dirty_cb(chan, chan->dirty_arg) != 0) {
			err("set_dirty %s: dirty callback failed\n",
					chan->name);
			return -1;
		}
	}

	return 0;
}

static int
check_duplicates(struct chan *chan, struct value *v)
{
	/* If duplicates are allowed just skip the check */
	if (chan->prop[CHAN_DUPLICATES])
		return 0;

	if (value_is_equal(&chan->last_value, v)) {
		err("check_duplicates %s: same value as last_value\n",
				chan->name);
		return -1;
	}

	return 0;
}

int
chan_set(struct chan *chan, struct value value)
{
	if (chan->type != CHAN_SINGLE) {
		err("chan_set %s: cannot set on non-single channel\n",
				chan->name);
		return -1;
	}

	if (chan->is_dirty && !chan->prop[CHAN_DIRTY_WRITE]) {
		err("chan_set %s: cannot modify dirty channel\n",
				chan->name);
		return -1;
	}

	if (check_duplicates(chan, &value) != 0) {
		err("chan_set %s: cannot set a duplicated value\n",
				chan->name);
		return -1;
	}

	//char buf[128];
	//dbg("chan_set %s: sets value to %s\n",
	//		chan->name, value_str(value, buf));
	chan->data.value = value;

	if (set_dirty(chan) != 0) {
		err("chan_set %s: set_dirty failed\n", chan->name);
		return -1;
	}

	return 0;
}

/** Adds one value to the stack. Fails if the stack is full.
 *
 *  @param ivalue The new integer value to be added on the stack.
 *
 *  @return On success returns 0, otherwise returns -1.
 */
int
chan_push(struct chan *chan, struct value value)
{
	if (chan->type != CHAN_STACK) {
		err("chan_push %s: cannot push on non-stack channel\n",
				chan->name);
		return -1;
	}

	if (chan->is_dirty && !chan->prop[CHAN_DIRTY_WRITE]) {
		err("chan_push %s: cannot modify dirty channel\n",
				chan->name);
		return -1;
	}

	if (check_duplicates(chan, &value) != 0) {
		err("chan_push %s: cannot push a duplicated value\n",
				chan->name);
		return -1;
	}

	struct chan_stack *stack = &chan->data.stack;

	if (stack->n >= MAX_CHAN_STACK) {
		err("chan_push %s: channel stack full\n", chan->name);
		return -1;
	}

	stack->values[stack->n++] = value;

	if (set_dirty(chan) != 0) {
		err("chan_push %s: set_dirty failed\n", chan->name);
		return -1;
	}

	return 0;
}

/** Remove one value from the stack. Fails if the top of the stack
 * doesn't match the expected value.
 *
 *  @param expected The expected value on the top of the stack.
 *
 *  @return On success returns 0, otherwise returns -1.
 */
int
chan_pop(struct chan *chan, struct value evalue)
{
	if (chan->type != CHAN_STACK) {
		err("chan_pop %s: cannot pop on non-stack channel\n",
				chan->name);
		return -1;
	}

	if (chan->is_dirty && !chan->prop[CHAN_DIRTY_WRITE]) {
		err("chan_pop %s: cannot modify dirty channel\n",
				chan->name);
		return -1;
	}

	struct chan_stack *stack = &chan->data.stack;

	if (stack->n <= 0) {
		err("chan_pop %s: channel stack empty\n", chan->name);
		return -1;
	}

	struct value *value = &stack->values[stack->n - 1];

	if (!value_is_equal(value, &evalue)) {
		err("chan_pop %s: unexpected value %ld (expected %ld)\n",
				chan->name, value->i, evalue.i);
		return -1;
	}

	stack->n--;

	if (set_dirty(chan) != 0) {
		err("chan_pop %s: set_dirty failed\n", chan->name);
		return -1;
	}

	return 0;
}

static void
get_value(struct chan *chan, struct value *value)
{
	if (chan->type == CHAN_SINGLE) {
		 *value = chan->data.value;
	} else {
		struct chan_stack *stack = &chan->data.stack;
		if (stack->n > 0)
			*value = stack->values[stack->n - 1];
		else
			*value = value_null();
	}
}

/** Reads the current value of a channel */
int
chan_read(struct chan *chan, struct value *value)
{
	get_value(chan, value);

	return 0;
}

/** Remove the dirty state */
int
chan_flush(struct chan *chan)
{
	if (!chan->is_dirty) {
		err("chan_flush %s: channel is not dirty\n", chan->name);
		return -1;
	}

	get_value(chan, &chan->last_value);

	chan->is_dirty = 0;

	return 0;
}

void
chan_prop_set(struct chan *chan, enum chan_prop prop, int value)
{
	chan->prop[prop] = value;
}

int
chan_prop_get(struct chan *chan, enum chan_prop prop)
{
	return chan->prop[prop];
}
