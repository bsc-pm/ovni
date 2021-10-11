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

void emit(struct ovni_stream *stream, struct ovni_ev *ev, int row)
{
	static uint64_t firstclock = 0;
	int64_t delta;
	int task;

	if(firstclock == 0)
		firstclock = ovni_ev_get_clock(ev);

	delta = ovni_ev_get_clock(ev) - firstclock;

	//#Paraver (19/01/38 at 03:14):00000000000000000000_ns:0:1:1(00000000000000000008:1)
	//2:0:1:1:7:1540663:6400010:1
	//2:0:1:1:7:1540663:6400015:1
	//2:0:1:1:7:1540663:6400017:0
	//2:0:1:1:7:1542091:6400010:1
	//2:0:1:1:7:1542091:6400015:1
	//2:0:1:1:7:1542091:6400025:1
	//2:0:1:1:7:1542091:6400017:0

	printf("2:0:1:1:%d:%ld:%d:%d\n", row, delta, ev->header.category, ev->header.value);
}

void dump_events(struct ovni_trace *trace)
{
	int i, f, row;
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

		if(lastclock >= ovni_ev_get_clock(stream->cur_ev))
		{
			fprintf(stderr, "warning: backwards jump in time\n");
		}

		/* Emit current event */
		row = f + 1;
		emit(stream, stream->cur_ev, row);

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
	struct ovni_trace *trace = calloc(1, sizeof(struct ovni_trace));

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

	printf("#Paraver (19/01/38 at 03:14):00000000000000000000_ns:0:1:1(%d:1)\n", trace->nstreams);

	dump_events(trace);

	ovni_free_streams(trace);

	fflush(stdout);

	free(trace);

	return 0;
}
