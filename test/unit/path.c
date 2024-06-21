/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu/path.h"
#include "common.h"
#include <string.h>

static void
test_underflow_trailing(void)
{
	char in[]  = { 'A', '/', '/', '/', '/', '\0', 'B' };
	char out[] = { 'A', '/', '\0','\0','\0','\0', 'B' };
	char *p = in + 2; /*      ^here */

	path_remove_trailing(p);

	if (memcmp(in, out, sizeof(in)) != 0) {
		for (size_t i = 0; i < sizeof(in); i++) {
			err("i=%3zd, in[i]=%02x out[i]=%02x", i,
					(unsigned char) in[i],
					(unsigned char) out[i]);
		}
		die("path mismatch");
	}

	err("OK");
}

int
main(void)
{
	test_underflow_trailing();

	return 0;
}
