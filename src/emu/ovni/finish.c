/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ovni_priv.h"

int
ovni_finish(struct emu *emu)
{
	/* Skip the check if the we are stopping prematurely */
	if (!emu->finished)
		return 0;

	struct system *sys = &emu->system;
	int ret = 0;

	/* Ensure that all threads are in the Dead state */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		if (t->state != TH_ST_DEAD) {
			err("thread %d is not dead (%s)", t->tid, t->id);
			ret = -1;
		}
	}

	return ret;
}
