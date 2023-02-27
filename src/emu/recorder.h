/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef RECORDER_H
#define RECORDER_H

/* Records data into files (Paraver traces only for now) */

#include "pv/pvt.h"

#include <linux/limits.h>

struct recorder {
	char dir[PATH_MAX]; /* To place the traces */
	struct pvt *pvt; /* Hash table by name */
};

USE_RET int recorder_init(struct recorder *rec, const char *dir);
USE_RET struct pvt *recorder_find_pvt(struct recorder *rec, const char *name);
USE_RET struct pvt *recorder_add_pvt(struct recorder *rec, const char *name, long nrows);
USE_RET int recorder_advance(struct recorder *rec, int64_t time);
USE_RET int recorder_finish(struct recorder *rec);

#endif /* RECORDER_H */
