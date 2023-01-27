/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ovni_model.h"

#include "emu.h"

int
ovni_model_probe(void *ptr)
{
	struct emu *emu = emu_get(ptr);

	if (emu->system.nthreads == 0)
		return -1;

	return 0;
}
