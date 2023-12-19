/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <ovni.h>
#include "instr.h"

/* Check the behavior of ovni_thread_isready(), which should only be 1
 * after ovni_thread_init() and before ovni_thread_free(). */

int
main(void)
{
	if (ovni_thread_isready())
		die("thread must not be ready before init");

	instr_start(0, 1);

	if (!ovni_thread_isready())
		die("thread must be ready after init");

	instr_end();

	if (ovni_thread_isready())
		die("thread must not be ready after free");

	return 0;
}
