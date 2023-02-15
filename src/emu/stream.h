/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_STREAM_H
#define EMU_STREAM_H

#include "ovni.h"
#include "heap.h"
#include <stdint.h>
#include <linux/limits.h>

struct stream;

struct stream {
	char path[PATH_MAX];
	char relpath[PATH_MAX]; /* To tracedir */

	uint8_t *buf;
	int64_t size;
	int64_t usize; /* Useful size for events */
	int64_t offset;

	int active;
	int unsorted;

	double progress;

	int64_t lastclock;
	int64_t clock_offset;

	heap_node_t hh;
	struct stream *next;
	struct stream *prev;

	struct ovni_ev *cur_ev;

	void *data; /* To hold system details */
};

int stream_load(struct stream *stream,
		const char *tracedir, const char *relpath);

int stream_clkoff_set(struct stream *stream, int64_t clock_offset);

double stream_progress(struct stream *stream);
int stream_step(struct stream *stream);
struct ovni_ev *stream_ev(struct stream *stream);
int64_t stream_evclock(struct stream *stream, struct ovni_ev *ev);
int64_t stream_lastclock(struct stream *stream);
void stream_allow_unsorted(struct stream *stream);

void stream_data_set(struct stream *stream, void *data);
void *stream_data_get(struct stream *stream);

#endif /* EMU_STREAM_H */
