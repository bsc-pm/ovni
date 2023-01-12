/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define ENABLE_DEBUG

#include "chan.h"
#include "common.h"
#include <string.h>

void
chan_init(struct chan *chan, enum chan_type type, const char *name)
{
	int len = strlen(name);
	if (len >= MAX_CHAN_NAME)
		die("chan_init: name '%s' too long\n", name);

	memset(chan, 0, sizeof(*chan));
	memcpy(chan->name, name, len + 1);
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
		err("channel %s already dirty\n", chan->name);
		return -1;
	}

	chan->is_dirty = 1;

	if (chan->dirty_cb != NULL) {
		if (chan->dirty_cb(chan, chan->dirty_arg) != 0) {
			err("dirty callback failed\n");
			return -1;
		}
	}

	return 0;
}

static int
check_duplicates(struct chan *chan, struct value *v)
{
	/* If duplicates are allowed just skip the check */
	//if (chan->oflags & CHAN_DUP)
	//	return 0;

	if (value_is_equal(&chan->last_value, v)) {
		err("check_duplicates: same value as last_value\n");
		return -1;
	}

	return 0;
}

int
chan_set(struct chan *chan, struct value value)
{
	if (chan->type != CHAN_SINGLE) {
		err("chan_set: cannot set on non-single channel\n");
		return -1;
	}

	if (chan->is_dirty) {
		err("chan_set: cannot modify dirty channel\n");
		return -1;
	}

	if (check_duplicates(chan, &value) != 0) {
		err("chan_set: cannot set a duplicated value\n");
		return -1;
	}

	char buf[128];
	dbg("chan_set: channel %p sets value %s\n", (void *) chan,
			value_str(value, buf));
	chan->data.value = value;

	if (set_dirty(chan) != 0) {
		err("chan_set: set_dirty failed\n");
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
		err("chan_push: cannot push on non-stack channel\n");
		return -1;
	}

	if (chan->is_dirty) {
		err("chan_push: cannot modify dirty channel\n");
		return -1;
	}

	if (check_duplicates(chan, &value) != 0) {
		err("chan_push: cannot push a duplicated value\n");
		return -1;
	}

	struct chan_stack *stack = &chan->data.stack;

	if (stack->n >= MAX_CHAN_STACK) {
		err("chan_push: channel stack full\n");
		abort();
	}

	stack->values[stack->n++] = value;

	if (set_dirty(chan) != 0) {
		err("chan_set: set_dirty failed\n");
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
		err("chan_pop: cannot pop on non-stack channel\n");
		return -1;
	}

	if (chan->is_dirty) {
		err("chan_pop: cannot modify dirty channel\n");
		return -1;
	}

	struct chan_stack *stack = &chan->data.stack;

	if (stack->n <= 0) {
		err("chan_pop: channel stack empty\n");
		return -1;
	}

	struct value *value = &stack->values[stack->n - 1];

	if (!value_is_equal(value, &evalue)) {
		err("chan_pop: unexpected value %ld (expected %ld)\n",
				value->i, evalue.i);
		return -1;
	}

	stack->n--;

	if (set_dirty(chan) != 0) {
		err("chan_set: set_dirty failed\n");
		return -1;
	}

	return 0;
}

static int
get_value(struct chan *chan, struct value *value)
{
	if (chan->type == CHAN_SINGLE) {
		 *value = chan->data.value;
		 return 0;
	}

	struct chan_stack *stack = &chan->data.stack;
	if (stack->n <= 0) {
		err("get_value: channel stack empty\n");
		return -1;
	}

	*value = stack->values[stack->n - 1];

	return 0;
}

/** Reads the current value of a channel */
int
chan_read(struct chan *chan, struct value *value)
{
	if (get_value(chan, value) != 0) {
		err("chan_read: get_value failed\n");
		return -1;
	}

	return 0;
}
