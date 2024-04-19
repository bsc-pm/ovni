/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "common.h"

#define VALUE_NBUF 32
#define VALUE_BUFSIZE 128

extern char value_buffers[VALUE_NBUF][VALUE_BUFSIZE];
extern size_t value_nextbuf;

enum value_type {
	VALUE_NULL = 0,
	VALUE_INT64,
	VALUE_DOUBLE
};

/* Ensure no padding holes */
struct value {
	int64_t type;
	union {
		int64_t i;
		double d;
	};
};

static inline int
value_is_equal(struct value *a, struct value *b)
{
	return memcmp(a, b, sizeof(struct value)) == 0;
}

static inline struct value
value_int64(int64_t i)
{
	struct value v = { .type = VALUE_INT64, .i = i };
	return v;
}

static inline struct value
value_null(void)
{
	struct value v = { .type = VALUE_NULL, .i = 0 };
	return v;
}

static inline int
value_is_null(struct value a)
{
	return (a.type == VALUE_NULL);
}

static inline char *
value_str(struct value a)
{
	char *buf = value_buffers[value_nextbuf];
	int ret = 0;
	size_t n = VALUE_BUFSIZE;

	switch (a.type) {
		case VALUE_NULL:
			ret = snprintf(buf, n, "{NULL}");
			break;
		case VALUE_INT64:
			ret = snprintf(buf, n, "{int64_t %" PRIi64 "}", a.i);
			break;
		case VALUE_DOUBLE:
			ret = snprintf(buf, n, "{double %e}", a.d);
			break;
		default:
			die("value_str: unexpected value type");
	}

	if (ret >= (int) n)
		die("value string too long");

	value_nextbuf++;
	if (value_nextbuf >= VALUE_NBUF)
		value_nextbuf = 0;

	return buf;
}

#endif /* VALUE_H */
