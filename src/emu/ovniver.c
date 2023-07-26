/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include "ovni.h"

int
main(void)
{
	printf("libovni version compiled %s, dynamic %s\n",
			OVNI_LIB_VERSION, ovni_version_get());

	return 0;
}
