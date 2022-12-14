/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

/* This program is a really bad idea. It attempts to sort streams by using a
 * window of the last N events in memory, so we can quickly access the event
 * clocks. Then, as soon as we detect a region of potentially unsorted events,
 * we go back until we find a suitable position and start injecting the events
 * in order.
 *
 * The events inside a unsorted region must be ordered. And the number of events
 * that we will look back is limited by N.
 */

#define _GNU_SOURCE

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "emu.h"
#include "ovni.h"
#include "trace.h"

struct ring {
	ssize_t head;
	ssize_t tail;
	ssize_t size;
	struct ovni_ev **ev;
};

struct sortplan {
	/* The first and last events which need sorting */
	struct ovni_ev *bad0;

	/* The next event which must be not affected */
	struct ovni_ev *next;

	/* Pointer to the stream buffer */
	uint8_t *base;

	struct ring *r;

	/* File descriptor of the stream file */
	int fd;
};

enum operation_mode { SORT,
	CHECK };

static char *tracedir = NULL;
static enum operation_mode operation_mode = SORT;
static const size_t max_look_back = 100000;

static void
ring_reset(struct ring *r)
{
	r->head = r->tail = 0;
}

static void
ring_add(struct ring *r, struct ovni_ev *ev)
{
	r->ev[r->tail] = ev;
	r->tail++;

	if (r->tail >= r->size)
		r->tail = 0;

	if (r->head == r->tail)
		r->head = r->tail + 1;

	if (r->head >= r->size)
		r->head = 0;
}

static ssize_t
find_destination(struct ring *r, uint64_t clock)
{
	ssize_t nback = 0;

	UNUSED(nback);

	ssize_t start = r->tail - 1 >= 0 ? r->tail - 1 : r->size - 1;
	ssize_t end = r->head - 1 >= 0 ? r->head - 1 : r->size - 1;
	uint64_t last_clock = 0;

	for (ssize_t i = start; i != end; i = i - 1 < 0 ? r->size - 1 : i - 1) {
		last_clock = r->ev[i]->header.clock;
		if (last_clock < clock) {
			dbg("found suitable position %ld events backwards\n",
					nback);
			return i;
		}
		nback++;
	}

	/* If there is no event with a lower clock and we haven't fill the ring
	 * yet, then we are at the beginning and no other event has be emitted
	 * before the sort window. So simply return the first marker. */
	if (nback < (ssize_t) r->size - 1) {
		if (r->head != 0)
			die("ring head expected to be 0\n");
		if (r->tail >= r->size - 1)
			die("ring tail=%ld expected to be less than %ld\n", r->tail, r->size - 1);

		dbg("starting of ring with nback=%ld\n", nback);
		return r->head;
	}

	err("cannot find a event previous to clock %lu\n", clock);
	err("nback=%ld, last_clock=%lu\n", nback, last_clock);

	return -1;
}

static int
starts_unsorted_region(struct ovni_ev *ev)
{
	return ev->header.model == 'O' && ev->header.category == 'U' && ev->header.value == '[';
}

static int
ends_unsorted_region(struct ovni_ev *ev)
{
	return ev->header.model == 'O' && ev->header.category == 'U' && ev->header.value == ']';
}

static void
sort_buf(uint8_t *src, uint8_t *buf, int64_t bufsize,
		uint8_t *srcbad, uint8_t *srcnext)
{
	int64_t injected = 0;

	UNUSED(injected);

	uint8_t *p = src;
	uint8_t *q = srcbad;

	while (1) {
		struct ovni_ev *ep = (struct ovni_ev *) p;
		struct ovni_ev *eq = (struct ovni_ev *) q;
		struct ovni_ev *ev = NULL;

		int64_t evsize = 0;

		if (p < srcbad && ep->header.clock < eq->header.clock) {
			ev = ep;
			evsize = ovni_ev_size(ev);
			p += evsize;
		} else {
			ev = eq;
			evsize = ovni_ev_size(ev);
			q += evsize;
		}

		if ((uint8_t *) ev == srcnext)
			break;

		if ((uint8_t *) ev > srcnext)
			die("exceeded srcnext while sorting\n");

		if (bufsize < evsize)
			die("no space left in the sort buffer\n");

		memcpy(buf, ev, evsize);
		buf += evsize;
		bufsize -= evsize;
		injected++;
	}

	dbg("injected %ld events in the past\n", injected);
}

static void
write_stream(int fd, void *base, void *dst, const void *src, size_t size)
{
	while (size > 0) {
		off_t offset = (off_t) ((int64_t) dst - (int64_t) base);
		ssize_t written = pwrite(fd, src, size, offset);

		if (written < 0)
			die("pwrite failed: %s\n", strerror(errno));

		size -= written;
		src = (void *) (((uint8_t *) src) + written);
		dst = (void *) (((uint8_t *) dst) + written);
	}
}

static int
execute_sort_plan(struct sortplan *sp)
{
	dbg("attempt to sort: start clock %ld\n", sp->bad0->header.clock);

	/* Cannot sort in one pass; just fail for now */
	int64_t i0 = find_destination(sp->r, sp->bad0->header.clock);
	if (i0 < 0) {
		err("cannot find destination for region starting at clock %ld\n",
				sp->bad0->header.clock);

		return -1;
	}

	/* Set the pointer to the first event that may be affected */
	struct ovni_ev *first = sp->r->ev[i0];

	/* Allocate a working buffer */
	int64_t bufsize = ((int64_t) sp->next) - ((int64_t) first);

	if (bufsize <= 0)
		die("bufsize is non-positive\n");

	uint8_t *buf = malloc(bufsize);
	if (!buf)
		die("malloc failed: %s\n", strerror(errno));

	sort_buf((uint8_t *) first, buf, bufsize,
			(uint8_t *) sp->bad0, (uint8_t *) sp->next);

	/* Copy the sorted events back into the stream buffer */
	memcpy(first, buf, bufsize);
	write_stream(sp->fd, sp->base, first, buf, bufsize);

	free(buf);

	return 0;
}

/* Sort the events in the stream chronologically using a ring */
static int
stream_winsort(struct ovni_stream *stream, struct ring *r)
{
	char *fn = stream->thread->tracefile;
	int fd = open(fn, O_WRONLY);

	if (fd < 0)
		die("open %s failed: %s\n", fn, strerror(errno));

	ring_reset(r);

	struct sortplan sp = {0};
	sp.r = r;
	sp.fd = fd;
	sp.base = stream->buf;

	size_t empty_regions = 0;
	size_t updated = 0;
	char st = 'S';

	while (ovni_load_next_event(stream) == 0) {
		struct ovni_ev *ev = stream->cur_ev;

		if (st == 'S' && starts_unsorted_region(ev)) {
			st = 'U';
		} else if (st == 'U') {
			/* Ensure that we have at least one unsorted
			 * event inside the section */
			if (ends_unsorted_region(ev)) {
				empty_regions++;
				st = 'S';
			} else {
				st = 'X';
				sp.bad0 = ev;
			}
		} else if (st == 'X') {
			if (ends_unsorted_region(ev)) {
				updated = 1;
				sp.next = ev;
				dbg("executing sort plan for stream tid=%d\n", stream->tid);
				if (execute_sort_plan(&sp) < 0) {
					err("sort failed for stream tid=%d\n",
							stream->tid);
					return -1;
				}

				/* Clear markers */
				sp.next = NULL;
				sp.bad0 = NULL;

				st = 'S';
			}
		}

		ring_add(r, ev);
	}

	if (empty_regions > 0)
		err("warning: stream %d contains %ld empty sort regions\n",
				stream->tid, empty_regions);

	if (updated && fdatasync(fd) < 0)
		die("fdatasync %s failed: %s\n", fn, strerror(errno));

	if (close(fd) < 0)
		die("close %s failed: %s\n", fn, strerror(errno));

	return 0;
}

/* Ensures that each individual stream is sorted */
static int
stream_check(struct ovni_stream *stream)
{
	if (ovni_load_next_event(stream) != 0)
		return 0;

	struct ovni_ev *ev = stream->cur_ev;
	uint64_t last_clock = ev->header.clock;
	int ret = 0;

	while (ovni_load_next_event(stream) == 0) {
		ev = stream->cur_ev;
		uint64_t cur_clock = ovni_ev_get_clock(ev);

		if (cur_clock < last_clock) {
			err("backwards jump in time %ld -> %ld for stream tid=%d\n",
					last_clock, cur_clock, stream->tid);
			ret = -1;
		}

		last_clock = cur_clock;
	}

	return ret;
}

static int
process_trace(struct ovni_trace *trace)
{
	struct ring ring;
	int ret = 0;

	ring.size = max_look_back;
	ring.ev = malloc(ring.size * sizeof(struct ovni_ev *));

	if (ring.ev == NULL)
		die("malloc failed: %s\n", strerror(errno));

	for (size_t i = 0; i < trace->nstreams; i++) {
		struct ovni_stream *stream = &trace->stream[i];
		if (operation_mode == SORT) {
			dbg("sorting stream tid=%d\n", stream->tid);
			if (stream_winsort(stream, &ring) != 0) {
				err("sort stream tid=%d failed\n", stream->tid);
				/* When sorting, return at the first
				 * attempt */
				return -1;
			}
		} else {
			if (stream_check(stream) != 0) {
				err("stream tid=%d is not sorted\n", stream->tid);
				/* When checking, report all errors and
				 * then fail */
				ret = -1;
			}
		}
	}

	free(ring.ev);

	if (operation_mode == CHECK) {
		if (ret == 0) {
			err("all streams sorted\n");
		} else {
			err("streams NOT sorted\n");
		}
	}

	return ret;
}

static void
usage(void)
{
	err("Usage: ovnisort [-c] tracedir\n");
	err("\n");
	err("Sorts the events in each stream of the trace given in\n");
	err("tracedir, so they are suitable for the emulator ovniemu.\n");
	err("Only the events enclosed by OU[ OU] are sorted. At most a\n");
	err("total of %ld events are looked back to insert the unsorted\n",
			max_look_back);
	err("events, so the sort procedure can fail with an error.\n");
	err("\n");
	err("Options:\n");
	err("  -c          Enable check mode: don't sort, ensure the\n");
	err("              trace is already sorted.\n");
	err("\n");
	err("  tracedir    The trace directory generated by ovni.\n");
	err("\n");

	exit(EXIT_FAILURE);
}

static void
parse_args(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "c")) != -1) {
		switch (opt) {
			case 'c':
				operation_mode = CHECK;
				break;
			default: /* '?' */
				usage();
		}
	}

	if (optind >= argc) {
		err("missing tracedir\n");
		usage();
	}

	tracedir = argv[optind];
}

int
main(int argc, char *argv[])
{
	int ret = 0;

	parse_args(argc, argv);

	struct ovni_trace *trace = calloc(1, sizeof(struct ovni_trace));

	if (trace == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	if (ovni_load_trace(trace, tracedir))
		return 1;

	if (ovni_load_streams(trace))
		return 1;

	if (process_trace(trace))
		ret = 1;

	ovni_free_streams(trace);

	ovni_free_trace(trace);

	free(trace);

	return ret;
}
