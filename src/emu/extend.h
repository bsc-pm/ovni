/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EXTEND_H
#define EXTEND_H

#include "common.h"

#define MAX_EXTEND 256

struct extend {
	void *ctx[MAX_EXTEND];
};

        void extend_set(struct extend *ext, int id, void *ctx);
USE_RET void *extend_get(struct extend *ext, int id);

#define EXT(st, m) extend_get(&(st)->ext, (m))

#endif /* EXTEND_H */
