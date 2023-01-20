/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_STREAM_H
#define EMU_STREAM_H

#include "heap.h"
#include <stdint.h>
#include <linux/limits.h>

struct emu_stream;

struct emu_stream {
	char path[PATH_MAX];
	char relpath[PATH_MAX]; /* To tracedir */

	uint8_t *buf;
	size_t size;
	size_t offset;

	int active;

	double progress;

	int64_t lastclock;
	int64_t clock_offset;

	heap_node_t hh;
	struct emu_stream *next;
	struct emu_stream *prev;

	void *data; /* To hold system details */
};

int emu_stream_load(struct emu_stream *stream,
		const char *tracedir, const char *relpath);

void emu_stream_clkoff(struct emu_stream *stream, int64_t clock_offset);

void emu_stream_data_set(struct emu_stream *stream, void *data);
void *emu_stream_data_get(struct emu_stream *stream);

#endif /* EMU_STREAM_H */
