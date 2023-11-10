/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <ovni.h>
#include "instr.h"

/* Emulate a crash in one thread by not calling ovni_thread_free(). We need to
 * call ovni_proc_fini() otherwise the metadata won't appear */

int
main(void)
{
	instr_start(0, 1);

	ovni_flush();
	ovni_proc_fini();

	return 0;
}
