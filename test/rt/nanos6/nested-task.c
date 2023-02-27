/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

int
main(void)
{
	#pragma oss task
	{
		#pragma oss task
		{
		}
		#pragma oss taskwait
	}
	return 0;
}
