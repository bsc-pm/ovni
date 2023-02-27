/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu/value.h"
#include "common.h"

int
main(void)
{
	struct value a, b;

	memset(&a, 66, sizeof(struct value));
	memset(&b, 0, sizeof(struct value));

	a = value_null();

	/* Ensure we can use the whole size of the value struct to
	 * compare two values, so we don't have problems with
	 * unitialized holes due to alignment */
	if (memcmp(&a, &b, sizeof(struct value)) != 0)
		die("values are not the same\n");

	err("OK\n");

	a = value_null();
	b = value_null();

	if (!value_is_equal(&a, &b))
		die("null not equal");

	return 0;
}
