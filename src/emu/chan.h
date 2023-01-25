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

enum chan_prop {
	CHAN_DIRTY_WRITE = 0,
	CHAN_DUPLICATES,
	CHAN_ROW,
	CHAN_MAXPROP,
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
	int prop[CHAN_MAXPROP];
	struct value last_value;
	chan_cb_t dirty_cb;
	void *dirty_arg;
};

        void chan_init(struct chan *chan, enum chan_type type, const char *fmt, ...);
USE_RET int chan_set(struct chan *chan, struct value value);
USE_RET int chan_push(struct chan *chan, struct value value);
USE_RET int chan_pop(struct chan *chan, struct value expected);
USE_RET int chan_read(struct chan *chan, struct value *value);
USE_RET enum chan_type chan_get_type(struct chan *chan);
USE_RET int chan_flush(struct chan *chan);
        void chan_prop_set(struct chan *chan, enum chan_prop prop, int value);
USE_RET int chan_prop_get(struct chan *chan, enum chan_prop prop);
        void chan_set_dirty_cb(struct chan *chan, chan_cb_t func, void *arg);

#endif /* CHAN_H */
