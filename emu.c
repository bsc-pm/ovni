#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <dirent.h> 
#include <assert.h> 

#include "ovni.h"
#include "ovni_trace.h"
#include "emu.h"

static void
emit_ev(struct ovni_stream *stream, struct ovni_ev *ev)
{
	int64_t delta;
	uint64_t clock;
	int i, payloadsize;

	//dbg("sizeof(*ev) = %d\n", sizeof(*ev));
	//hexdump((uint8_t *) ev, sizeof(*ev));

	clock = ovni_ev_get_clock(ev);

	delta = clock - stream->lastclock;

	dbg("%d.%d.%d %c %c %c % 20lu % 15ld ",
			stream->loom, stream->proc, stream->tid,
			ev->model, ev->class, ev->value, clock, delta);

	payloadsize = ovni_payload_size(ev);
	for(i=0; i<payloadsize; i++)
	{
		dbg("%d ", ev->payload.u8[i]);
	}
	dbg("\n");

	stream->lastclock = clock;
}

void
emu_emit(struct ovni_emu *emu)
{
	emit_ev(emu->cur_stream, emu->cur_ev);
}

static void
load_first_event(struct ovni_stream *stream)
{
	int i;
	size_t n;
	struct ovni_ev *ev;

	if(!stream->active)
		return;

	ev = &stream->last;
	if((n = fread(ev, sizeof(*ev), 1, stream->f)) != 1)
	{
		//fprintf(stderr, "failed to read an event, n=%ld\n", n);
		stream->active = 0;
		return;
	}

	stream->active = 1;
}

struct ovni_ethread *
find_thread(struct ovni_eproc *proc, pid_t tid)
{
	int i;
	struct ovni_ethread *thread;

	for(i=0; i<proc->nthreads; i++)
	{
		thread = &proc->thread[i];
		if(thread->tid == tid)
			return thread;
	}

	return NULL;
}

static void
hook_pre(struct ovni_emu *emu)
{
	//emu_emit(emu);

	switch(emu->cur_ev->model)
	{
		case 'O': hook_pre_ovni(emu); break;
		default:
			  //dbg("unknown model %c\n", emu->cur_ev->model);
			  break;
	}
}

static void
hook_view(struct ovni_emu *emu)
{
	//emu_emit(emu);

	switch(emu->cur_ev->model)
	{
		case 'O': hook_view_ovni(emu); break;
		default:
			  //dbg("unknown model %c\n", emu->cur_ev->model);
			  break;
	}
}

static void
hook_post(struct ovni_emu *emu)
{
	//emu_emit(emu);

	switch(emu->cur_ev->model)
	{
		case 'O': hook_post_ovni(emu); break;
		default:
			  //dbg("unknown model %c\n", emu->cur_ev->model);
			  break;
	}
}

static void
set_current(struct ovni_emu *emu, struct ovni_stream *stream)
{
	emu->cur_stream = stream;
	emu->cur_ev = &stream->last;
	emu->cur_loom = &emu->trace.loom[stream->loom];
	emu->cur_proc = &emu->cur_loom->proc[stream->proc];
	emu->cur_thread = &emu->cur_proc->thread[stream->thread];
}

static int
next_event(struct ovni_emu *emu)
{
	int i, f;
	uint64_t minclock;
	struct ovni_ev *ev;
	struct ovni_stream *stream;
	struct ovni_trace *trace;

	trace = &emu->trace;

	f = -1;
	minclock = 0;

	/* TODO: use a heap */

	/* Select next event based on the clock */
	for(i=0; i<trace->nstreams; i++)
	{
		stream = &trace->stream[i];

		if(!stream->active)
			continue;

		ev = &stream->last;
		if(f < 0 || ovni_ev_get_clock(ev) < minclock)
		{
			f = i;
			minclock = ovni_ev_get_clock(ev);
		}
	}

	if(f < 0)
		return -1;

	/* We have a valid stream with a new event */
	stream = &trace->stream[f];

	set_current(emu, stream);

	if(emu->lastclock > ovni_ev_get_clock(&stream->last))
	{
		fprintf(stdout, "warning: backwards jump in time %lu -> %lu\n",
				emu->lastclock, ovni_ev_get_clock(&stream->last));
	}

	emu->lastclock = ovni_ev_get_clock(&stream->last);

	return 0;
}

static void
emulate(struct ovni_emu *emu)
{
	int i;
	struct ovni_ev ev;
	struct ovni_stream *stream;
	struct ovni_trace *trace;

	/* Load events */
	trace = &emu->trace;
	for(i=0; i<trace->nstreams; i++)
	{
		stream = &trace->stream[i];
		ovni_load_next_event(stream);
	}

	emu->lastclock = 0;

	/* Then process all events */
	while(next_event(emu) == 0)
	{
		//fprintf(stdout, "step %i\n", i);
		hook_pre(emu);
		hook_view(emu);
		hook_post(emu);

		/* Read the next event */
		ovni_load_next_event(emu->cur_stream);
	}
}

struct ovni_ethread *
emu_get_thread(struct ovni_emu *emu, int tid)
{
	int i, j, k;
	struct ovni_loom *loom;
	struct ovni_eproc *proc;
	struct ovni_ethread *thread;

	for(i=0; i<emu->trace.nlooms; i++)
	{
		loom = &emu->trace.loom[i];
		for(j=0; j<loom->nprocs; j++)
		{
			proc = &loom->proc[j];
			for(k=0; k<proc->nthreads; k++)
			{
				thread = &proc->thread[k];
				if(thread->tid == tid)
				{
					/* Only same process threads can
					 * change the affinity to each
					 * others */
					assert(emu->cur_proc == proc);
					return thread;
				}
			}
		}
	}

	return thread;
}

int
main(int argc, char *argv[])
{
	char *tracedir;
	struct ovni_emu emu;

	if(argc != 2)
	{
		fprintf(stderr, "missing tracedir\n");
		exit(EXIT_FAILURE);
	}

	tracedir = argv[1];

	memset(&emu, 0, sizeof(emu));

	if(ovni_load_trace(&emu.trace, tracedir))
		return 1;

	if(ovni_load_streams(&emu.trace))
		return 1;

	emulate(&emu);

	ovni_free_streams(&emu.trace);

	return 0;
}
