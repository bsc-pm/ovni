/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ovni_priv.h"

struct model_spec model_ovni = {
	.name = "ovni",
	.model = 'O',
	.create  = NULL,
	.connect = NULL,
	.event   = ovni_event,
	.probe   = ovni_probe,
};

int
ovni_probe(struct emu *emu)
{
	if (emu->system.nthreads == 0)
		return -1;

	return 0;
}
