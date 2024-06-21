/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "chan.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "common.h"

void __attribute__((format(printf, 3, 4)))
chan_init(struct chan *chan, enum chan_type type, const char *fmt, ...)
{
	memset(chan, 0, sizeof(struct chan));

	va_list ap;
	va_start(ap, fmt);

	int n = ARRAYLEN(chan->name);
	int ret = vsnprintf(chan->name, n, fmt, ap);
	if (ret >= n)
		die("channel name too long");
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

		err("%s: already dirty", chan->name);
		return -1;
	}

	chan->is_dirty = 1;

	if (chan->dirty_cb != NULL) {
		dbg("%s: calling dirty callback", chan->name);
		if (chan->dirty_cb(chan, chan->dirty_arg) != 0) {
			err("%s: dirty callback failed", chan->name);
			return -1;
		}
	}

	return 0;
}

int
chan_set(struct chan *chan, struct value value)
{
	if (chan->type != CHAN_SINGLE) {
		err("%s: cannot set on non-single channel", chan->name);
		return -1;
	}

	if (chan->is_dirty && !chan->prop[CHAN_DIRTY_WRITE]) {
		err("%s: cannot modify dirty channel", chan->name);
		return -1;
	}

	/* If duplicates are allowed just skip the check */
	if (!chan->prop[CHAN_ALLOW_DUP]) {
		if (value_is_equal(&chan->last_value, &value)) {
			if (chan->prop[CHAN_IGNORE_DUP]) {
				dbg("%s: value already set to %s",
						chan->name, value_str(value));
				return 0;
			} else {
				err("%s: same value as last_value %s",
						chan->name, value_str(chan->last_value));
				return -1;
			}
		}
	}

	dbg("%s: sets value to %s", chan->name, value_str(value));
	chan->data.value = value;

	if (set_dirty(chan) != 0) {
		err("%s: set_dirty failed", chan->name);
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
		err("%s: cannot push on non-stack channel", chan->name);
		return -1;
	}

	if (chan->is_dirty && !chan->prop[CHAN_DIRTY_WRITE]) {
		err("%s: cannot modify dirty channel", chan->name);
		return -1;
	}

	/* If duplicates are allowed just skip the check */
	if (!chan->prop[CHAN_ALLOW_DUP]) {
		if (value_is_equal(&chan->last_value, &value)) {
			if (chan->prop[CHAN_IGNORE_DUP]) {
				dbg("%s: value already set to %s",
						chan->name, value_str(value));
				return 0;
			} else {
				err("%s: same value as last_value %s",
						chan->name, value_str(chan->last_value));
				return -1;
			}
		}
	}

	struct chan_stack *stack = &chan->data.stack;

	if (stack->n >= MAX_CHAN_STACK) {
		err("%s: channel stack full", chan->name);
		return -1;
	}

	stack->values[stack->n++] = value;

	if (set_dirty(chan) != 0) {
		err("%s: set_dirty failed", chan->name);
		return -1;
	}

	return 0;
}

/** Remove one value from the stack. Fails if the top of the stack
 * doesn't match the expected value.
 *
 *  @param evalue The expected value on the top of the stack.
 *
 *  @return On success returns 0, otherwise returns -1.
 */
int
chan_pop(struct chan *chan, struct value evalue)
{
	if (chan->type != CHAN_STACK) {
		err("%s: cannot pop on non-stack channel", chan->name);
		return -1;
	}

	if (chan->is_dirty && !chan->prop[CHAN_DIRTY_WRITE]) {
		err("%s: cannot modify dirty channel", chan->name);
		return -1;
	}

	struct chan_stack *stack = &chan->data.stack;

	if (stack->n <= 0) {
		err("%s: channel stack empty", chan->name);
		return -1;
	}

	struct value *value = &stack->values[stack->n - 1];

	if (!value_is_equal(value, &evalue)) {
		err("%s: expected value %s different from top of stack %s",
				chan->name,
				value_str(evalue),
				value_str(*value));
		return -1;
	}

	stack->n--;

	if (set_dirty(chan) != 0) {
		err("%s: set_dirty failed", chan->name);
		return -1;
	}

	return 0;
}

static void
get_value(struct chan *chan, struct value *value)
{
	if (likely(chan->type == CHAN_SINGLE)) {
		 *value = chan->data.value;
	} else {
		struct chan_stack *stack = &chan->data.stack;
		if (stack->n > 0)
			*value = stack->values[stack->n - 1];
		else
			*value = value_null();
	}
}

/** Remove the dirty state */
int
chan_flush(struct chan *chan)
{
	if (!chan->is_dirty) {
		err("%s: channel is not dirty", chan->name);
		return -1;
	}

	get_value(chan, &chan->last_value);

	chan->is_dirty = 0;

	return 0;
}

/** Marks the channel as dirty */
int
chan_dirty(struct chan *chan)
{
	/* Nothing to do, already dirty */
	if (chan->is_dirty)
		return 0;

	if (set_dirty(chan) != 0) {
		err("%s: set_dirty failed", chan->name);
		return -1;
	}

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
