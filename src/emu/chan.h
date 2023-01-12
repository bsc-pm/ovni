/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef CHAN_H
#define CHAN_H

#include <stdint.h>
#include "value.h"

#define MAX_CHAN_STACK 512
#define MAX_CHAN_NAME 512

enum chan_type {
	CHAN_SINGLE = 0,
	CHAN_STACK = 1,
	CHAN_MAXTYPE,
};

struct chan_stack {
	int n;
	struct value values[MAX_CHAN_STACK];
};

union chan_data {
	struct chan_stack stack;
	struct value value;
};

struct chan;

typedef int (*chan_cb_t)(struct chan *chan, void *ptr);

struct chan {
	char name[MAX_CHAN_NAME];
	enum chan_type type;
	union chan_data data;
	int is_dirty;
	struct value err_value;
	struct value last_value;
	chan_cb_t dirty_cb;
	void *dirty_arg;
};

//int chan_enable(struct chan *chan);
//int chan_disable(struct chan *chan);
//int chan_is_enabled(const struct chan *chan);

void chan_init(struct chan *chan, enum chan_type type, const char *name);
int chan_set(struct chan *chan, struct value value);
int chan_push(struct chan *chan, struct value value);
int chan_pop(struct chan *chan, struct value expected);
int chan_read(struct chan *chan, struct value *value);
enum chan_type chan_get_type(struct chan *chan);

/* Called when it becomes dirty */
void chan_set_dirty_cb(struct chan *chan, chan_cb_t func, void *arg);

#endif /* CHAN_H */
