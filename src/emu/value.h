#ifndef VALUE_H
#define VALUE_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "common.h"

enum value_type {
	VALUE_NULL = 0,
	VALUE_INT64,
	VALUE_DOUBLE
};

/* Packed allows the struct to be hashable, as we don't have any
 * unitialized data */
struct __attribute__((packed)) value {
	enum value_type type;
	union {
		int64_t i;
		double d;
	};
};

static inline int
value_is_equal(struct value *a, struct value *b)
{
	if (a->type != b->type)
		return 0;

	if (a->type == VALUE_INT64 && a->i == b->i)
		return 1;
	else if (a->type == VALUE_DOUBLE && a->d == b->d)
		return 1;
	else
		return 0;
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
value_str(struct value a, char *buf)
{
	switch (a.type) {
		case VALUE_NULL:
			sprintf(buf, "{NULL}");
			break;
		case VALUE_INT64:
			sprintf(buf, "{int64_t %ld}", a.i);
			break;
		case VALUE_DOUBLE:
			sprintf(buf, "{double %e}", a.d);
			break;
		default:
			die("value_str: unexpected value type\n");
	}

	return buf;
}

#endif /* VALUE_H */