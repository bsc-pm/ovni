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
#include "ovni_trace.h"
#include "emu.h"


static void
hexdump(uint8_t *buf, size_t size)
{
	int i, j;

	//printf("writing %ld bytes in cpu=%d\n", size, rthread.cpu);

	for(i=0; i<size; i+=16)
	{
		for(j=0; j<16 && i+j < size; j++)
		{
			fprintf(stderr, "%02x ", buf[i+j]);
		}
		fprintf(stderr, "\n");
	}
}

void emit(struct ovni_stream *stream, struct ovni_ev *ev)
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
			ev->header.model, ev->header.class, ev->header.value, clock, delta);

	payloadsize = ovni_payload_size(ev);
	for(i=0; i<payloadsize; i++)
	{
		printf("%02x ", ev->payload.u8[i]);
	}
	printf("\n");

	stream->lastclock = clock;
}


void dump_events(struct ovni_trace *trace)
{
	int i, f;
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
	struct ovni_trace trace;

	if(argc != 2)
	{
		fprintf(stderr, "missing tracedir\n");
		exit(EXIT_FAILURE);
	}

	tracedir = argv[1];

	if(ovni_load_trace(&trace, tracedir))
		return 1;

	if(ovni_load_streams(&trace))
		return 1;

	dump_events(&trace);

	ovni_free_streams(&trace);

	return 0;
}
