/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu/pv/cfg.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "common.h"

int main(void)
{
	if (cfg_generate(".") != 0)
		die("cfg_generate failed");

	/* Check that one configuration file is present */
	struct stat st;
	if (stat("cfg/cpu/ovni/pid.cfg", &st) != 0)
		die("stat failed");

	return 0;
}
