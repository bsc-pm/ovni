/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: MIT */

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "common.h"

static inline int
version_parse(const char *version, int tuple[3])
{
	char buf[64];

	if (version == NULL) {
		err("version is NULL");
		return -1;
	}

	if (strlen(version) >= 64) {
		err("version too long: %s", version);
		return -1;
	}

	strcpy(buf, version);

	char *str = buf;
	char *which[] = { "major", "minor", "patch" };
	char *delim[] = { ".", ".", ".-" };
	char *save = NULL;

	for (int i = 0; i < 3; i++) {
		char *num = strtok_r(str, delim[i], &save);

		/* Subsequent calls need NULL as string */
		str = NULL;

		if (num == NULL) {
			err("missing %s number: %s",
					which[i], version);
			return -1;
		}

		errno = 0;
		char *endptr = NULL;
		int v = (int) strtol(num, &endptr, 10);

		if (errno != 0 || endptr == num || endptr[0] != '\0') {
			err("failed to parse %s number: %s",
					which[i], version);
			return -1;
		}

		if (v < 0) {
			err("invalid negative %s number: %s",
					which[i], version);
			return -1;
		}

		tuple[i] = v;
	}

	return 0;
}
