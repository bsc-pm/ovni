/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "player.h"
#include <stdlib.h>
#include <string.h>
#include "heap.h"
#include "stream.h"
#include "trace.h"
#include "utlist.h"

/*
 * heap_node_compare_t - comparison function, returns:

 * 	> 0 if a > b
 * 	< 0 if a < b
 * 	= 0 if a == b
 *
 * Invert the comparison function to get a min-heap instead
 */
static inline int
stream_cmp(heap_node_t *a, heap_node_t *b)
{
	struct stream *sa, *sb;

	sa = heap_elem(a, struct stream, hh);
	sb = heap_elem(b, struct stream, hh);

	int64_t ca = stream_lastclock(sa);
	int64_t cb = stream_lastclock(sb);

	/* Return the opposite, so we have min-heap */
	if (ca < cb)
		return +1;
	else if (ca > cb)
		return -1;
	else
		return 0;
}

static int
step_stream(struct player *player, struct stream *stream)
{
	if (!stream->active)
		return +1;

	int ret = stream_step(stream);

	if (ret < 0) {
		err("cannot step stream '%s'", stream->relpath);
		return -1;
	} else if (ret > 0) {
		return ret;
	}

	heap_insert(&player->heap, &stream->hh, &stream_cmp);

	player->nprocessed++;

	return 0;
}

static int
check_clock_gate(struct trace *trace)
{
	/* 1 hour in nanoseconds */
	int64_t maxgate = 3600LL * 1000LL * 1000LL * 1000LL;
	int64_t t0 = 0LL;
	int first = 1;
	int ret = 0;

	struct stream *stream;
	DL_FOREACH(trace->streams, stream) {
		struct ovni_ev *oev = stream_ev(stream);
		int64_t sclock = stream_evclock(stream, oev);

		if (first) {
			first = 0;
			t0 = sclock;
		}

		int64_t delta = llabs(t0 - sclock);
		if (delta > maxgate) {
			double hdelta = ((double) delta) / (3600.0 * 1e9);
			err("stream %s has starting clock too far: delta=%.2f h",
					stream->relpath, hdelta);
			ret = -1;
		}
	}

	if (ret != 0) {
		err("detected large clock gate, run 'ovnisync' to set the offsets");
		return -1;
	}

	return 0;
}

int
player_init(struct player *player, struct trace *trace, int unsorted)
{
	memset(player, 0, sizeof(struct player));

	heap_init(&player->heap);

	player->first_event = 1;
	player->stream = NULL;
	player->trace = trace;
	player->unsorted = unsorted;

	/* Load initial streams and events */
	struct stream *stream;
	DL_FOREACH(trace->streams, stream) {
		if (unsorted)
			stream_allow_unsorted(stream);

		int ret = step_stream(player, stream);
		if (ret > 0) {
			/* No more events */
			continue;
		} else if (ret < 0) {
			err("step_stream failed");
			return -1;
		}
	}

	/* Ensure the first event sclocks are not too far apart. Otherwise an
	 * offset table is mandatory. */
	if (unsorted == 0 && check_clock_gate(trace) != 0) {
		err("check_clock_gate failed");
		return -1;
	}

	return 0;
}

static int
update_clocks(struct player *player, struct stream *stream)
{
	/* This can happen if two events are not ordered in the stream, but the
	 * emulator picks other events in the middle. Example:
	 *
	 * Stream A: 10 3 ...
	 * Stream B: 5 12
	 *
	 * emulator output:
	 * 5
	 * 10
	 * 3  -> error!
	 * 12
	 * ...
	 * */
	int64_t sclock = stream_lastclock(stream);

	if (player->first_event) {
		player->first_event = 0;
		player->firstclock = sclock;
		player->lastclock = sclock;
	}

	if (sclock < player->lastclock) {
		err("backwards jump in time %ld -> %ld in stream '%s'",
				player->lastclock, sclock, stream->relpath);
		if (player->unsorted == 0)
			return -1;
	}

	player->lastclock = sclock;
	player->deltaclock = player->lastclock - player->firstclock;

	return 0;
}

/* Returns -1 on error, +1 if there are no more events and 0 if next event
 * loaded properly */
int
player_step(struct player *player)
{
	/* Add the stream back if still active */
	if (player->stream != NULL && step_stream(player, player->stream) < 0) {
		err("step_stream() failed");
		return -1;
	}

	/* Extract the next stream based on the lastclock */
	heap_node_t *node = heap_pop_max(&player->heap, stream_cmp);

	/* No more streams */
	if (node == NULL)
		return +1;

	struct stream *stream = heap_elem(node, struct stream, hh);

	if (stream == NULL) {
		err("heap_elem() returned NULL");
		return -1;
	}

	if (update_clocks(player, stream) != 0) {
		err("update_clocks() failed");
		return -1;
	}

	player->stream = stream;

	struct ovni_ev *oev = stream_ev(stream);
	emu_ev(&player->ev, oev, player->lastclock, player->deltaclock);

	return 0;
}

struct emu_ev *
player_ev(struct player *player)
{
	return &player->ev;
}

struct stream *
player_stream(struct player *player)
{
	return player->stream;
}

double
player_progress(struct player *player)
{
	struct trace *trace = player->trace;
	struct stream *stream;
	int64_t sum_done = 0;
	int64_t sum_total = 0;
	DL_FOREACH(trace->streams, stream) {
		int64_t done, total;
		stream_progress(stream, &done, &total);
		sum_done += done;
		sum_total += total;
	}

	if (sum_total == 0)
		return 1.0;

	return (double) sum_done / (double) sum_total;
}

int64_t
player_nprocessed(struct player *player)
{
	return player->nprocessed;
}
