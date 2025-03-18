/* Copyright (c) 2021-2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

/* This program is a really bad idea. It attempts to sort streams by using a
 * window of the last N events in memory, so we can quickly access the event
 * clocks. Then, as soon as we detect a region of potentially unsorted events,
 * we go back until we find a suitable position and start injecting the events
 * in order.
 *
 * The events inside a unsorted region may not be ordered, they will be sorted
 * by qsort() first. The number of events that we will look back is limited by
 * N.
 */

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "ovni.h"
#include "stream.h"
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
static size_t max_look_back = 1000000;

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

static void
ring_check(struct ring *r, long long start)
{
	uint64_t last_clock = 0;
	for (long long i = start; i != r->tail; i = (i + 1) % r->size) {
		uint64_t clock = r->ev[i]->header.clock;
		if (clock < last_clock) {
			die("ring not sorted at i=%lld, last_clock=%"PRIu64" clock=%"PRIu64 ,
					i, last_clock, clock);
		}
		last_clock = clock;
	}
}

static ssize_t
find_destination(struct ring *r, uint64_t clock)
{
	ssize_t nback = 0;

	ssize_t start = r->tail - 1 >= 0 ? r->tail - 1 : r->size - 1;
	ssize_t end = r->head - 1 >= 0 ? r->head - 1 : r->size - 1;
	uint64_t last_clock = 0;

	for (ssize_t i = start; i != end; i = i - 1 < 0 ? r->size - 1 : i - 1) {
		last_clock = r->ev[i]->header.clock;
		if (last_clock < clock) {
			dbg("found suitable position %zd events backwards",
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
			die("ring head expected to be 0");
		if (r->tail >= r->size - 1)
			die("ring tail=%zd expected to be less than %zd", r->tail, r->size - 1);

		dbg("starting of ring with nback=%zd", nback);
		return r->head;
	}

	err("cannot find a event previous to clock %"PRIu64, clock);
	err("nback=%zd, last_clock=%"PRIu64, nback, last_clock);

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

static uint64_t
find_min_clock(uint8_t *src, uint8_t *end)
{
	uint8_t *p = src;
	struct ovni_ev *ev0 = (struct ovni_ev *) p;
	uint64_t min_clock = ev0->header.clock;

	while (1) {
		if (p >= end)
			break;

		struct ovni_ev *ev = (struct ovni_ev *) p;
		if (ev->header.clock < min_clock)
			min_clock = ev->header.clock;

		p += ovni_ev_size(ev);
	}

	return min_clock;
}

static long
count_events(uint8_t *src, uint8_t *end)
{
	uint8_t *p = src;
	long n = 0;

	while (1) {
		if (p >= end)
			break;

		struct ovni_ev *ev = (struct ovni_ev *) p;
		p += ovni_ev_size(ev);
		n++;
	}

	return n;
}

static void
index_events(struct ovni_ev **table, long n, uint8_t *buf)
{
	uint8_t *p = buf;

	for (long i = 0; i < n; i++) {
		table[i] = (struct ovni_ev *) p;
		p += ovni_ev_size(table[i]);
	}
}

static void
write_events(struct ovni_ev **table, long n, uint8_t *buf)
{
	for (long i = 0; i < n; i++) {
		struct ovni_ev *ev = table[i];
		size_t size = (size_t) ovni_ev_size(ev);
		memcpy(buf, ev, size);
		buf += size;

		dbg("injected event %c%c%c at %"PRIu64,
				ev->header.model,
				ev->header.category,
				ev->header.value,
				ev->header.clock);
	}
}

static int
cmp_ev(const void *a, const void *b)
{
	struct ovni_ev **pev1 = (struct ovni_ev **) a;
	struct ovni_ev **pev2 = (struct ovni_ev **) b;

	struct ovni_ev *ev1 = *pev1;
	struct ovni_ev *ev2 = *pev2;

	int64_t clock1 = (int64_t) ev1->header.clock;
	int64_t clock2 = (int64_t) ev2->header.clock;

	if (clock1 < clock2)
		return -1;
	if (clock1 > clock2)
		return +1;
	else
		return 0;
}

static void
sort_buf(uint8_t *src, uint8_t *buf, int64_t bufsize)
{
	struct ovni_ev *ev = (struct ovni_ev *) src;

	dbg("first event before sorting %c%c%c at %"PRIu64,
			ev->header.model,
			ev->header.category,
			ev->header.value,
			ev->header.clock);

	/* Create a copy of the array */
	uint8_t *buf2 = malloc((size_t) bufsize);
	if (buf2 == NULL)
		die("malloc failed:");

	memcpy(buf2, src, (size_t) bufsize);

	long n = count_events(buf2, buf2 + bufsize);
	struct ovni_ev **table = calloc((size_t) n, sizeof(struct ovni_ev *));
	if (table == NULL)
		die("calloc failed:");

	index_events(table, n, buf2);
	qsort(table, (size_t) n, sizeof(struct ovni_ev *), cmp_ev);
	write_events(table, n, buf);

	dbg("first event after sorting %c%c%c at %"PRIu64,
			ev->header.model,
			ev->header.category,
			ev->header.value,
			ev->header.clock);

	free(table);
	free(buf2);

	dbg("sorted %ld events", n);
}

static void
write_stream(int fd, void *base, void *dst, const void *src, size_t size)
{
	while (size > 0) {
		off_t offset = (off_t) ((intptr_t) dst - (intptr_t) base);
		ssize_t written = pwrite(fd, src, size, offset);

		if (written < 0)
			die("pwrite failed:");

		size -= (size_t) written;
		src = (void *) (((uint8_t *) src) + written);
		dst = (void *) (((uint8_t *) dst) + written);
	}
}

static void
rebuild_ring(struct ring *r, long long start, struct ovni_ev *first, struct ovni_ev *last)
{
	long long nbad = 0;
	long long n = 0;
	struct ovni_ev *ev = first;

	for (long long i = start; i != r->tail; i = i + 1 >= r->size ? 0 : i + 1) {
		n++;
		if (ev != r->ev[i])
			nbad++;

		if (ev >= last)
			die("exceeding last pointer");

		r->ev[i] = ev;
		size_t size = (size_t) ovni_ev_size(ev);
		ev = (struct ovni_ev *) (((uint8_t *) ev) + size);
	}

	if (ev != last)
		die("inconsistency: ev != last");

	dbg("rebuilt ring with %lld / %lld misplaced events", nbad, n);
}

static int
execute_sort_plan(struct sortplan *sp)
{
	uint64_t clock0 = sp->bad0->header.clock;
	dbg("attempt to sort: start clock %"PRIi64, sp->bad0->header.clock);

	uint64_t min_clock = find_min_clock((void *) sp->bad0, (void *) sp->next);

	if (min_clock < clock0) {
		clock0 = min_clock;
		dbg("region not sorted, using min clock=%"PRIi64, clock0);
	}

	/* Cannot sort in one pass; just fail for now */
	int64_t i0 = find_destination(sp->r, clock0);
	if (i0 < 0) {
		err("cannot find destination for region starting at clock %"PRIi64, clock0);
		err("consider increasing the look back size with -n");
		return -1;
	}

	/* Set the pointer to the first event that may be affected */
	struct ovni_ev *first = sp->r->ev[i0];
	long long dirty = i0;

	/* Allocate a working buffer */
	uintptr_t bufsize = (uintptr_t) sp->next - (uintptr_t) first;

	if (bufsize <= 0)
		die("bufsize is non-positive");

	uint8_t *buf = malloc(bufsize);
	if (!buf)
		die("malloc failed:");

	sort_buf((uint8_t *) first, buf, (int64_t) bufsize);

	write_stream(sp->fd, sp->base, first, buf, bufsize);

	free(buf);

	/* Pointers from the ring buffer are invalid now, rebuild them */
	rebuild_ring(sp->r, dirty, first, sp->next);

	/* Invariant: The ring buffer is always sorted here. Check from the
	 * dirty position onwards, so we avoid scanning all events. */
	ring_check(sp->r, dirty);

	return 0;
}

/* Sort the events in the stream chronologically using a ring */
static int
stream_winsort(struct stream *stream, struct ring *r)
{
	char *fn = stream->obspath;
	int fd = open(fn, O_WRONLY);

	if (fd < 0)
		die("open %s failed:", fn);

	ring_reset(r);

	struct sortplan sp = {0};
	sp.r = r;
	sp.fd = fd;
	sp.base = stream->buf;

	size_t empty_regions = 0;
	size_t updated = 0;
	char st = 'S';
	int ret = 0;

	while ((ret = stream_step(stream)) == 0) {
		struct ovni_ev *ev = stream_ev(stream);

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
				dbg("executing sort plan for stream %s",
						stream->relpath);
				if (execute_sort_plan(&sp) < 0) {
					err("sort failed for stream %s",
							stream->relpath);
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

	if (ret < 0) {
		err("stream_step failed");
		return -1;
	}

	if (empty_regions > 0)
		warn("stream %s contains %zd empty sort regions",
				stream->relpath, empty_regions);

	if (updated && fdatasync(fd) < 0)
		die("fdatasync %s failed:", fn);

	if (close(fd) < 0)
		die("close %s failed:", fn);

	return 0;
}

/* Ensures that each individual stream is sorted */
static int
stream_check(struct stream *stream)
{
	int ret = stream_step(stream);
	if (ret < 0) {
		err("stream_step failed");
		return -1;
	}

	/* Reached the end */
	if (ret != 0)
		return 0;

	struct ovni_ev *ev = stream_ev(stream);
	uint64_t last_clock = ev->header.clock;
	int backjump = 0;

	while ((ret = stream_step(stream)) == 0) {
		ev = stream_ev(stream);
		uint64_t cur_clock = ovni_ev_get_clock(ev);

		if (cur_clock < last_clock) {
			err("backwards jump in time %"PRIi64" -> %"PRIi64" for stream %s",
					last_clock, cur_clock, stream->relpath);
			backjump = 1;
		}

		last_clock = cur_clock;
	}

	if (ret < 0) {
		err("stream_step failed");
		return -1;
	}

	if (backjump)
		return -1;

	return 0;
}

static int
process_trace(struct trace *trace)
{
	struct ring ring;
	int ret = 0;

	ring.size = (ssize_t) max_look_back;
	ring.ev = malloc((size_t) ring.size * sizeof(struct ovni_ev *));

	if (ring.ev == NULL)
		die("malloc failed:");

	for (struct stream *stream = trace->streams; stream; stream = stream->next) {
		stream_allow_unsorted(stream);

		if (operation_mode == SORT) {
			dbg("sorting stream %s", stream->relpath);
			if (stream_winsort(stream, &ring) != 0) {
				err("sort stream %s failed", stream->relpath);
				/* When sorting, return at the first
				 * attempt */
				return -1;
			}
		} else {
			if (stream_check(stream) != 0) {
				info("stream %s is not sorted", stream->relpath);
				/* When checking, report all errors and
				 * then fail */
				ret = -1;
			}
		}
	}

	free(ring.ev);

	if (operation_mode == CHECK) {
		if (ret == 0) {
			info("all streams sorted");
		} else {
			info("streams NOT sorted");
		}
	}

	return ret;
}

static void
usage(void)
{
	rerr("Usage: ovnisort [-c] tracedir\n");
	rerr("\n");
	rerr("Sorts the events in each stream of the trace given in\n");
	rerr("tracedir, so they are suitable for the emulator ovniemu.\n");
	rerr("Only the events enclosed by OU[ OU] are sorted. At most a\n");
	rerr("total of %zd events are looked back to insert the unsorted\n",
			max_look_back);
	rerr("events, so the sort procedure can fail with an error.\n");
	rerr("\n");
	rerr("Options:\n");
	rerr("  -c          Enable check mode: don't sort, ensure the\n");
	rerr("              trace is already sorted.\n");
	rerr("\n");
	rerr("  -n          Set the number of events to look back.\n");
	rerr("              Default: %zd\n", max_look_back);
	rerr("\n");
	rerr("  tracedir    The trace directory generated by ovni.\n");
	rerr("\n");

	exit(EXIT_FAILURE);
}

static void
parse_args(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "cn:")) != -1) {
		switch (opt) {
			case 'c':
				operation_mode = CHECK;
				break;
			case 'n':
				max_look_back = (size_t) atol(optarg);
				break;
			default: /* '?' */
				usage();
		}
	}

	if (optind >= argc) {
		err("missing tracedir");
		usage();
	}

	tracedir = argv[optind];
}

int
main(int argc, char *argv[])
{
	progname_set("ovnisort");

	if (getenv("OVNI_DEBUG") != NULL)
		enable_debug();

	parse_args(argc, argv);

	struct trace *trace = calloc(1, sizeof(struct trace));

	if (trace == NULL) {
		err("calloc failed:");
		return 1;
	}

	if (trace_load(trace, tracedir) != 0) {
		err("failed to load trace: %s", tracedir);
		return 1;
	}

	int ret = process_trace(trace);

	free(trace);

	if (ret)
		return 1;

	return 0;
}
