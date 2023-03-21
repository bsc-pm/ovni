/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu/value.h"
#include "common.h"

/* Ensure there are no holes in the value structure */
static void
test_holes(void)
{
	struct value a, b;

	memset(&a, 66, sizeof(struct value));
	memset(&b, 0, sizeof(struct value));

	a = value_null();

	/* Ensure we can use the whole size of the value struct to
	 * compare two values, so we don't have problems with
	 * unitialized holes due to alignment */
	if (memcmp(&a, &b, sizeof(struct value)) != 0)
		die("values are not the same");

	err("OK");
}

/* Ensure value_null results in values being equal */
static void
test_null_init(void)
{
	struct value a, b;

	memset(&a, 66, sizeof(struct value));
	memset(&b, 0, sizeof(struct value));

	a = value_null();
	b = value_null();

	if (!value_is_equal(&a, &b))
		die("null not equal");

	err("OK");
}

/* Test that we can printf at least 8 values without overwritting the
 * value_str() buffer */
static void
test_multiple_str(void)
{
	if (VALUE_NBUF < 8)
		die("VALUE_NBUF too small");

	char *str[VALUE_NBUF];
	for (int i = 0; i < VALUE_NBUF; i++) {
		struct value v = value_int64(i);
		str[i] = value_str(v);
	}

	for (int i = 0; i < VALUE_NBUF; i++) {
		char expected[128];
		sprintf(expected, "{int64_t %d}", i);
		if (strcmp(expected, str[i]) != 0) {
			die("mismatch in bufer i=%d: expected %s, got %s",
					i, expected, str[i]);
		}
	}

	err("OK");
}


int
main(void)
{
	test_holes();
	test_null_init();
	test_multiple_str();

	return 0;
}
