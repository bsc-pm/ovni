/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "extend.h"

void
extend_set(struct extend *ext, int id, void *ctx)
{
	ext->ctx[id] = ctx;
}

void *
extend_get(struct extend *ext, int id)
{
	return ext->ctx[id];
}
