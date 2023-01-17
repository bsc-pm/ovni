/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_TRACE_H
#define EMU_TRACE_H

#include "emu_stream.h"

#include <linux/limits.h>

struct emu_trace {
	char tracedir[PATH_MAX];

	long nstreams;
	struct emu_stream *streams;

	heap_head_t sorted_stream;
};

int emu_trace_load(struct emu_trace *trace, const char *tracedir);

#endif /* EMU_TRACE_H */
