/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef METADATA_H
#define METADATA_H

#include "common.h"
struct loom;
struct proc;

USE_RET int metadata_load(const char *path, struct loom *loom, struct proc *proc);

#endif /* METADATA_H */
