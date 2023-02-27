/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "version.h"

struct testcase {
	int rc;
	char *version;
	int tuple[3];
};

int main(void)
{
	struct testcase cases[] = {
		/* Good */
		{ 0, "0.0.0", 		{ 0, 0, 0 } },
		{ 0, "1.0.0", 		{ 1, 0, 0 } },
		{ 0, "0.1.0", 		{ 0, 1, 0 } },
		{ 0, "0.0.1", 		{ 0, 0, 1 } },
		{ 0, "1.2.3-rc1", 	{ 1, 2, 3 } },
		/* Bad */
		{ -1, "-1.0.0",		{ 0, 0, 0 } },
		{ -1, "1.2", 		{ 0, 0, 0 } },
		{ -1, "1",		{ 0, 0, 0 } },
		{ -1, "1.O.O",		{ 0, 0, 0 } },
		{ -1, "1.2.3rc",	{ 0, 0, 0 } },
		{ -1, NULL,		{ 0, 0, 0 } },
	};

	int n = sizeof(cases) / sizeof(cases[0]);

	for (int i = 0; i < n; i++) {
		struct testcase *c = &cases[i];
		int tuple[3] = { 0 };

		if (version_parse(c->version, tuple) != c->rc)
			die("wrong return value\n");

		if (c->rc != 0)
			continue;

		for (int j = 0; j < 3; j++) {
			if (tuple[j] != c->tuple[j])
				die("wrong parsed version\n");
		}
	}

	err("ok");

	return 0;
}
