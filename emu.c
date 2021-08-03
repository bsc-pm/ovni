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
#include "prv.h"

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
			ev->header.model, ev->header.class, ev->header.value, clock, delta);

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

	switch(emu->cur_ev->header.model)
	{
		case 'O': hook_pre_ovni(emu); break;
		case 'V': hook_pre_nosv(emu); break;
		default:
			  //dbg("unknown model %c\n", emu->cur_ev->model);
			  break;
	}
}

static void
hook_emit(struct ovni_emu *emu)
{
	//emu_emit(emu);

	switch(emu->cur_ev->header.model)
	{
		case 'O': hook_emit_ovni(emu); break;
		case 'V': hook_emit_nosv(emu); break;
		default:
			  //dbg("unknown model %c\n", emu->cur_ev->model);
			  break;
	}
}

static void
hook_post(struct ovni_emu *emu)
{
	//emu_emit(emu);

	switch(emu->cur_ev->header.model)
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
	emu->cur_ev = stream->cur_ev;
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
	static int64_t t0 = -1;

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

		ev = stream->cur_ev;
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

	if(emu->lastclock > ovni_ev_get_clock(stream->cur_ev))
	{
		fprintf(stdout, "warning: backwards jump in time %lu -> %lu\n",
				emu->lastclock, ovni_ev_get_clock(stream->cur_ev));
	}

	emu->lastclock = ovni_ev_get_clock(stream->cur_ev);

	if(t0 < 0)
		t0 = emu->lastclock;

	emu->delta_time = emu->lastclock - t0;

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
		hook_emit(emu);
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

static void
add_new_cpu(struct ovni_emu *emu, struct ovni_loom *loom, int i, int phyid)
{
	/* The logical id must match our index */
	assert(i == loom->ncpus);

	assert(loom->cpu[i].state == CPU_ST_UNKNOWN);

	loom->cpu[loom->ncpus].state = CPU_ST_READY;
	loom->cpu[loom->ncpus].i = i;
	loom->cpu[loom->ncpus].phyid = phyid;
	loom->cpu[loom->ncpus].gindex = emu->total_cpus++;

	dbg("new cpu %d at phyid=%d\n", i, phyid);

	loom->ncpus++;
}

void
proc_load_cpus(struct ovni_emu *emu, struct ovni_loom *loom, struct ovni_eproc *proc)
{
	JSON_Array *cpuarray;
	JSON_Object *cpu;
	JSON_Object *meta;
	int i, index, phyid;

	meta = json_value_get_object(proc->meta);

	assert(meta);

	cpuarray = json_object_get_array(meta, "cpus");

	/* This process doesn't have the cpu list */
	if(cpuarray == NULL)
		return;

	assert(loom->ncpus == 0);

	for(i = 0; i < json_array_get_count(cpuarray); i++)
	{
		cpu = json_array_get_object(cpuarray, i);

		index = (int) json_object_get_number(cpu, "index");
		phyid = (int) json_object_get_number(cpu, "phyid");

		add_new_cpu(emu, loom, index, phyid);
	}
}

/* Obtain CPUs in the metadata files and other data */
static int
load_metadata(struct ovni_emu *emu)
{
	int i, j, k, s;
	struct ovni_loom *loom;
	struct ovni_eproc *proc;
	struct ovni_ethread *thread;
	struct ovni_stream *stream;
	struct ovni_trace *trace;

	trace = &emu->trace;

	for(i=0; i<trace->nlooms; i++)
	{
		loom = &trace->loom[i];
		for(j=0; j<loom->nprocs; j++)
		{
			proc = &loom->proc[j];

			proc_load_cpus(emu, loom, proc);
		}

		/* One of the process must have the list of CPUs */
		/* FIXME: The CPU list should be at the loom dir */
		assert(loom->ncpus > 0);
	}

	return 0;
}

static int
destroy_metadata(struct ovni_emu *emu)
{
	int i, j, k, s;
	struct ovni_loom *loom;
	struct ovni_eproc *proc;
	struct ovni_ethread *thread;
	struct ovni_stream *stream;
	struct ovni_trace *trace;

	trace = &emu->trace;

	for(i=0; i<trace->nlooms; i++)
	{
		loom = &trace->loom[i];
		for(j=0; j<loom->nprocs; j++)
		{
			proc = &loom->proc[j];

			assert(proc->meta);
			json_value_free(proc->meta);
		}
	}

	return 0;
}

static void
open_prvs(struct ovni_emu *emu, char *tracedir)
{
	char path[PATH_MAX];

	sprintf(path, "%s/%s", tracedir, "thread.prv");

	emu->prv_thread = fopen(path, "w");

	if(emu->prv_thread == NULL)
		abort();

	sprintf(path, "%s/%s", tracedir, "cpu.prv");

	emu->prv_cpu = fopen(path, "w");

	if(emu->prv_cpu == NULL)
		abort();

	prv_header(emu->prv_thread, emu->trace.nstreams);
	prv_header(emu->prv_cpu, emu->total_cpus + 1);
}

static void
close_prvs(struct ovni_emu *emu)
{
	fclose(emu->prv_thread);
	fclose(emu->prv_cpu);
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
		abort();

	if(ovni_load_streams(&emu.trace))
		abort();

	if(load_metadata(&emu) != 0)
		abort();

	open_prvs(&emu, tracedir);

	printf("#Paraver (19/01/38 at 03:14):00000000000000000000_ns:0:1:1(%d:1)\n", emu.total_cpus);

	emulate(&emu);

	close_prvs(&emu);

	destroy_metadata(&emu);

	ovni_free_streams(&emu.trace);


	return 0;
}
