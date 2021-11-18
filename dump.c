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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <dirent.h> 

#include "ovni.h"
#include "trace.h"
#include "emu.h"

//static void
//hexdump(uint8_t *buf, size_t size)
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
//}

static void
emit(struct ovni_stream *stream, struct ovni_ev *ev)
{
	int64_t delta;
	uint64_t clock;
	int i, payloadsize;

	//printf("sizeof(*ev) = %d\n", sizeof(*ev));
	//hexdump((uint8_t *) ev, sizeof(*ev));

	clock = ovni_ev_get_clock(ev);

	delta = clock - stream->lastclock;

	printf("%d.%d.%d %c %c %c % 20ld % 15ld ",
			stream->loom, stream->proc, stream->tid,
			ev->header.model, ev->header.category, ev->header.value, clock, delta);

	payloadsize = ovni_payload_size(ev);
	for(i=0; i<payloadsize; i++)
	{
		printf("%02x ", ev->payload.u8[i]);
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
	for(i=0; i<trace->nstreams; i++)
	{
		stream = &trace->stream[i];
		ovni_load_next_event(stream);
	}

	lastclock = 0;

	while(1)
	{
		f = -1;
		minclock = 0;

		/* Select next event based on the clock */
		for(i=0; i<trace->nstreams; i++)
		{
			stream = &trace->stream[i];

			if(!stream->active)
				continue;

			ev = stream->cur_ev;
			if(f < 0 || ovni_ev_get_clock(ev) < minclock)
			{
				f = i;
				minclock = ovni_ev_get_clock(ev);
			}
		}

		//fprintf(stderr, "f=%d minclock=%u\n", f, minclock);

		if(f < 0)
			break;

		stream = &trace->stream[f];

		if(lastclock > ovni_ev_get_clock(stream->cur_ev))
		{
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

int main(int argc, char *argv[])
{
	char *tracedir;
	struct ovni_trace *trace;

	trace = calloc(1, sizeof(struct ovni_trace));

	if(trace == NULL)
	{
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	if(argc != 2)
	{
		fprintf(stderr, "missing tracedir\n");
		exit(EXIT_FAILURE);
	}

	tracedir = argv[1];

	if(ovni_load_trace(trace, tracedir))
		return 1;

	if(ovni_load_streams(trace))
		return 1;

	dump_events(trace);

	ovni_free_streams(trace);

	free(trace);

	return 0;
}
