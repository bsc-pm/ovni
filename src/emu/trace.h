/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_TRACE_H
#define EMU_TRACE_H

#include <limits.h>
#include "common.h"

struct trace {
	char tracedir[PATH_MAX];

	long nstreams;
	struct stream *streams;
};

USE_RET int trace_load(struct trace *trace, const char *tracedir);

#endif /* EMU_TRACE_H */
