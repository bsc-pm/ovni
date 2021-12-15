/*
 * Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

#include "ovni.h"
#include "trace.h"
#include "emu.h"

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

	struct ring *r;
};

enum operation_mode { SORT, CHECK };

static char *tracedir = NULL;
static enum operation_mode operation_mode = SORT;
static const size_t max_look_back = 10000;

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

	if(r->tail >= r->size)
		r->tail = 0;

	if(r->head == r->tail)
		r->head = r->tail + 1;

	if(r->head >= r->size)
		r->head = 0;
}

static ssize_t
find_destination(struct ring *r, uint64_t clock)
{
	ssize_t i, start, end, nback = 0;

	start = r->tail - 1 >= 0 ? r->tail - 1 : r->size - 1;
	end = r->head - 1 >= 0 ? r->head - 1 : r->size - 1;

	for(i=start; i != end; i = i-1 < 0 ? r->size - 1: i-1)
	{
		if(r->ev[i]->header.clock < clock)
		{
			dbg("found suitable position %ld events backwards\n",
					nback);
			return i;
		}
		nback++;
	}

	err("cannot find a event previous to clock %lu\n", clock);

	return -1;
}

static int
starts_unsorted_region(struct ovni_ev *ev)
{
	return ev->header.model == 'O' &&
		ev->header.category == 'U' &&
		ev->header.value == '[';
}

static int
ends_unsorted_region(struct ovni_ev *ev)
{
	return ev->header.model == 'O' &&
		ev->header.category == 'U' &&
		ev->header.value == ']';
}

static void
hexdump(uint8_t *buf, size_t size)
{
	UNUSED(buf);
	UNUSED(size);

#ifdef ENABLE_DEBUG

	size_t i, j;

	//printf("writing %ld bytes in cpu=%d\n", size, rthread.cpu);

	for(i=0; i<size; i+=16)
	{
		for(j=0; j<16; j++)
		{
			if(i+j < size)
				fprintf(stderr, "%02x ", buf[i+j]);
			else
				fprintf(stderr, "   ");
		}

		fprintf(stderr, " | ");

		for(j=0; j<16; j++)
		{
			if(i+j < size && isprint(buf[i+j]))
				fprintf(stderr, "%c", buf[i+j]);
			else
				fprintf(stderr, " ");
		}
		fprintf(stderr, "\n");
	}

#endif
}

static void
sort_buf(uint8_t *src, uint8_t *buf, int64_t bufsize,
		uint8_t *srcbad, uint8_t *srcnext)
{
	uint8_t *p, *q, *obuf=buf;
	int64_t evsize, injected = 0, obufsize=bufsize;
	struct ovni_ev *ep, *eq, *ev;

	p = src;
	q = srcbad;

	dbg("src before:\n");
	hexdump(src, obufsize);

	while(1)
	{
		ep = (struct ovni_ev *) p;
		eq = (struct ovni_ev *) q;

		if(p < srcbad && ep->header.clock < eq->header.clock)
		{
			ev = ep;
			evsize = ovni_ev_size(ev);
			p += evsize;
		}
		else
		{
			ev = eq;
			evsize = ovni_ev_size(ev);
			q += evsize;
		}

		if((uint8_t *) ev == srcnext)
			break;

		if((uint8_t *) ev > srcnext)
			die("exceeded srcnext while sorting\n");

		if(bufsize < evsize)
			die("no space left in the sort buffer\n");

		memcpy(buf, ev, evsize);
		buf += evsize;
		bufsize -= evsize;
		injected++;
	}

	dbg("buf after:\n");
	hexdump(obuf, obufsize);

	dbg("injected %ld events in the past\n", injected);
}

static int
execute_sort_plan(struct sortplan *sp)
{
	int64_t i0, bufsize;
	uint8_t *buf;
	/* The first event in the stream that may be affected */
	struct ovni_ev *first;


	dbg("attempt to sort: start clock %ld\n", sp->bad0->header.clock);

	/* Cannot sort in one pass; just fail for now */
	if((i0 = find_destination(sp->r, sp->bad0->header.clock)) < 0)
	{
		err("cannot find a destination for unsorted region\n");
		return -1;
	}

	/* Set the pointer to the first event */
	first = sp->r->ev[i0];

	/* Allocate a working buffer */
	bufsize = ((int64_t) sp->next) - ((int64_t) first);

	if(bufsize <= 0)
		die("bufsize is non-positive\n");

	buf = malloc(bufsize);
	if(!buf)
		die("malloc failed: %s\n", strerror(errno));

	sort_buf((uint8_t *) first, buf, bufsize,
			(uint8_t *) sp->bad0, (uint8_t *) sp->next);

	/* Copy the sorted events back into the stream buffer */
	memcpy(first, buf, bufsize);
	free(buf);

	return 0;
}

/* Sort the events in the stream chronologically using a ring */
static int
stream_winsort(struct ovni_stream *stream, struct ring *r)
{
	ssize_t i;
	struct ovni_ev *ev;
	struct sortplan sp = {0};
	//uint64_t lastclock = 0;
	char st = 'S';

	ring_reset(r);
	sp.r = r;

	for(i=0; stream->active; i++)
	{
		ovni_load_next_event(stream);
		ev = stream->cur_ev;

		if(st == 'S' && starts_unsorted_region(ev))
		{
			st = 'U';
		}
		else if(st == 'U')
		{
			/* Ensure that we have at least one unsorted
			 * event inside the section */
			if(ends_unsorted_region(ev))
			{
				err("warning: ignoring empty sort section\n");
				st = 'S';
			}
			else
			{
				st = 'X';
				sp.bad0 = ev;
			}
		}
		else if(st == 'X')
		{
			if(ends_unsorted_region(ev))
			{
				sp.next = ev;
				dbg("executing sort plan for stream tid=%d\n", stream->tid);
				if(execute_sort_plan(&sp) < 0)
				{
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

	return 0;
}

/* Ensures that each individual stream is sorted */
static int
stream_check(struct ovni_stream *stream)
{
	ovni_load_next_event(stream);
	struct ovni_ev *ev = stream->cur_ev;
	uint64_t last_clock = ev->header.clock;
	int ret = 0;

	for(ssize_t i=0; stream->active; i++)
	{
		ovni_load_next_event(stream);
		ev = stream->cur_ev;
		uint64_t cur_clock = ovni_ev_get_clock(ev);

		if(cur_clock < last_clock)
		{
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
	size_t i;
	struct ovni_stream *stream;
	struct ring ring;
	int ret = 0;

	ring.size = max_look_back;
	ring.ev = malloc(ring.size * sizeof(struct ovni_ev *));

	if(ring.ev == NULL)
		die("malloc failed: %s\n", strerror(errno));

	for(i=0; i<trace->nstreams; i++)
	{
		stream = &trace->stream[i];
		if(operation_mode == SORT)
		{
			dbg("sorting stream tid=%d\n", stream->tid);
			if(stream_winsort(stream, &ring) != 0)
			{
				err("sort stream tid=%d failed\n", stream->tid);
				/* When sorting, return at the first
				 * attempt */
				return -1;
			}
		}
		else
		{
			if(stream_check(stream) != 0)
			{
				err("stream tid=%d is not sorted\n", stream->tid);
				/* When checking, report all errors and
				 * then fail */
				ret = -1;
			}
		}
	}

	free(ring.ev);

	return ret;
}

static void
usage(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

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

	while((opt = getopt(argc, argv, "c")) != -1)
	{
		switch(opt)
		{
			case 'c':
				operation_mode = CHECK;
				break;
			default: /* '?' */
				usage(argc, argv);
		}
	}

	if(optind >= argc)
	{
		err("missing tracedir\n");
		usage(argc, argv);
	}

	tracedir = argv[optind];
}

int main(int argc, char *argv[])
{
	int ret = 0;
	struct ovni_trace *trace;

	parse_args(argc, argv);

	trace = calloc(1, sizeof(struct ovni_trace));

	if(trace == NULL)
	{
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	if(ovni_load_trace(trace, tracedir))
		return 1;

	if(ovni_load_streams(trace))
		return 1;

	if(process_trace(trace))
		ret = 1;

	ovni_free_streams(trace);

	free(trace);

	return ret;
}
