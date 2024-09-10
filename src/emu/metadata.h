/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef METADATA_H
#define METADATA_H

#include "common.h"
struct loom;
struct proc;
struct thread;

USE_RET int metadata_load_proc(const char *path, struct loom *loom, struct proc *proc);

#endif /* METADATA_H */
