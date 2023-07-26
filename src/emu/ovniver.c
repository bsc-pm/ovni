/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include "ovni.h"

int
main(void)
{
	const char *version, *commit;
	ovni_version_get(&version, &commit);

	printf("libovni: build v%s (%s), dynamic v%s (%s)\n",
			OVNI_LIB_VERSION, OVNI_GIT_COMMIT,
			version, commit);

	return 0;
}
