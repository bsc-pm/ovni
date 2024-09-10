/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef STREAM_H
#define STREAM_H

#include <limits.h>
#include <stdint.h>
#include "common.h"
#include "heap.h"
#include "parson.h"
struct ovni_ev;

struct stream {
	struct ovni_ev *cur_ev;
	uint8_t *buf;
	int64_t size;
	int64_t lastclock;
	int64_t deltaclock;
	int64_t clock_offset;

	heap_node_t hh;
	struct stream *next;
	struct stream *prev;

	void *data; /* To hold system details */

	int active;
	int unsorted;

	char path[PATH_MAX]; /* To stream dir */
	char relpath[PATH_MAX]; /* To tracedir */
	char obspath[PATH_MAX]; /* To obs file */
	char jsonpath[PATH_MAX]; /* To json file */

	int64_t usize; /* Useful size for events */
	int64_t offset;

	double progress;

	JSON_Object *meta;
};

USE_RET int stream_load(struct stream *stream, const char *tracedir, const char *relpath);
USE_RET int stream_clkoff_set(struct stream *stream, int64_t clock_offset);
        void stream_progress(struct stream *stream, int64_t *done, int64_t *total);
USE_RET int stream_step(struct stream *stream);
USE_RET struct ovni_ev *stream_ev(struct stream *stream);
USE_RET int64_t stream_evclock(struct stream *stream, struct ovni_ev *ev);
USE_RET int64_t stream_lastclock(struct stream *stream);
        void stream_allow_unsorted(struct stream *stream);
        void stream_data_set(struct stream *stream, void *data);
USE_RET void *stream_data_get(struct stream *stream);
USE_RET JSON_Object *stream_metadata(struct stream *stream);

#endif /* STREAM_H */
