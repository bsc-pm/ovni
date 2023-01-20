/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_STREAM_H
#define EMU_STREAM_H

#include "ovni.h"
#include "heap.h"
#include <stdint.h>
#include <linux/limits.h>

struct emu_stream;

struct emu_stream {
	char path[PATH_MAX];
	char relpath[PATH_MAX]; /* To tracedir */

	uint8_t *buf;
	int64_t size;
	int64_t usize; /* Useful size for events */
	int64_t offset;

	int active;

	double progress;

	int64_t lastclock;
	int64_t clock_offset;

	heap_node_t hh;
	struct emu_stream *next;
	struct emu_stream *prev;

	struct ovni_ev *cur_ev;

	void *data; /* To hold system details */
};

int emu_stream_load(struct emu_stream *stream,
		const char *tracedir, const char *relpath);

int emu_stream_clkoff_set(struct emu_stream *stream, int64_t clock_offset);

double emu_stream_progress(struct emu_stream *stream);
int emu_stream_step(struct emu_stream *stream);
struct ovni_ev *emu_stream_ev(struct emu_stream *stream);
int64_t emu_stream_evclock(struct emu_stream *stream, struct ovni_ev *ev);
int64_t emu_stream_lastclock(struct emu_stream *stream);

void emu_stream_data_set(struct emu_stream *stream, void *data);
void *emu_stream_data_get(struct emu_stream *stream);

#endif /* EMU_STREAM_H */
