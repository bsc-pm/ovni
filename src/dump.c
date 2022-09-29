/* Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "emu.h"
#include "ovni.h"
#include "trace.h"

int filter_tid = -1;
char *tracedir;

// static void
// hexdump(uint8_t *buf, size_t size)
//{
//	size_t i, j;
//
//	//printf("writing %ld bytes in cpu=%d\n", size, rthread.cpu);
//
//	for(i=0; i<size; i+=16)
//	{
//		for(j=0; j<16 && i+j < size; j++)
//		{
//			fprintf(stderr, "%02x ", buf[i+j]);
//		}
//		fprintf(stderr, "\n");
//	}
// }

static void
emit(struct ovni_stream *stream, struct ovni_ev *ev)
{
	uint64_t clock;
	int i, payloadsize;

	// printf("sizeof(*ev) = %d\n", sizeof(*ev));
	// hexdump((uint8_t *) ev, sizeof(*ev));

	clock = ovni_ev_get_clock(ev);

	printf("%s.%d.%d  %ld  %c%c%c",
		stream->loom->hostname,
		stream->proc->pid,
		stream->thread->tid,
		clock,
		ev->header.model,
		ev->header.category,
		ev->header.value);

	payloadsize = ovni_payload_size(ev);
	if (payloadsize > 0) {
		printf(" ");
		for (i = 0; i < payloadsize; i++)
			printf(":%02x", ev->payload.u8[i]);
	}
	printf("\n");

	stream->lastclock = clock;
}


static void
dump_events(struct ovni_trace *trace)
{
	size_t i;
	ssize_t f;
	uint64_t minclock, lastclock;
	struct ovni_ev *ev;
	struct ovni_stream *stream;

	/* Load events */
	for (i = 0; i < trace->nstreams; i++) {
		stream = &trace->stream[i];

		/* It can be inactive if it has been disabled by the
		 * thread TID filter */
		if (stream->active)
			ovni_load_next_event(stream);
	}

	lastclock = 0;

	while (1) {
		f = -1;
		minclock = 0;

		/* Select next event based on the clock */
		for (i = 0; i < trace->nstreams; i++) {
			stream = &trace->stream[i];

			if (!stream->active)
				continue;

			ev = stream->cur_ev;
			if (f < 0 || ovni_ev_get_clock(ev) < minclock) {
				f = i;
				minclock = ovni_ev_get_clock(ev);
			}
		}

		// fprintf(stderr, "f=%d minclock=%u\n", f, minclock);

		if (f < 0)
			break;

		stream = &trace->stream[f];

		if (lastclock > ovni_ev_get_clock(stream->cur_ev)) {
			fprintf(stdout, "warning: backwards jump in time %lu -> %lu\n",
				lastclock, ovni_ev_get_clock(stream->cur_ev));
		}

		/* Emit current event */
		emit(stream, stream->cur_ev);

		lastclock = ovni_ev_get_clock(stream->cur_ev);

		/* Read the next one */
		ovni_load_next_event(stream);

		/* Unset the index */
		f = -1;
		minclock = 0;
	}
}

static void
usage(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	err("Usage: ovnidump [-t TID] tracedir\n");
	err("\n");
	err("Dumps the events of the trace to the standard output.\n");
	err("\n");
	err("Options:\n");
	err("  -t TID      Only events from the given TID are shown\n");
	err("\n");
	err("  tracedir    The trace directory generated by ovni.\n");
	err("\n");

	exit(EXIT_FAILURE);
}

static void
parse_args(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "t:")) != -1) {
		switch (opt) {
			case 't':
				filter_tid = atoi(optarg);
				break;
			default: /* '?' */
				usage(argc, argv);
		}
	}

	if (optind >= argc) {
		err("missing tracedir\n");
		usage(argc, argv);
	}

	tracedir = argv[optind];
}

int
main(int argc, char *argv[])
{
	parse_args(argc, argv);

	struct ovni_trace *trace = calloc(1, sizeof(struct ovni_trace));

	if (ovni_load_trace(trace, tracedir))
		return 1;

	if (ovni_load_streams(trace))
		return 1;

	if (filter_tid != -1) {
		for (size_t i = 0; i < trace->nstreams; i++) {
			struct ovni_stream *stream;
			stream = &trace->stream[i];
			if (stream->tid != filter_tid)
				stream->active = 0;
		}
	}

	dump_events(trace);

	ovni_free_streams(trace);

	free(trace);

	return 0;
}