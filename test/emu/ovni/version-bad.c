/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ovni.h"

#undef OVNI_LIB_VERSION
#define OVNI_LIB_VERSION "0.0.0" /* Cause the major to fail */

int
main(void)
{
	/* Should fail */
	ovni_version_check();
	return 0;
}
