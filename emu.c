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

#define _POSIX_C_SOURCE 200112L

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
#include <unistd.h>
#include <time.h>

#include "ovni.h"
#include "trace.h"
#include "emu.h"
#include "prv.h"
#include "pcf.h"
#include "chan.h"
#include "utlist.h"

/* Obtains the corrected clock of the given event */
static int64_t
evclock(struct ovni_stream *stream, struct ovni_ev *ev)
{
	return (int64_t) ovni_ev_get_clock(ev) + stream->clock_offset;
}

static void
print_ev(struct ovni_stream *stream, struct ovni_ev *ev)
{
	int64_t clock, delta = 0;
	int i, payloadsize;

	UNUSED(delta);

	//dbg("sizeof(*ev) = %d\n", sizeof(*ev));
	//hexdump((uint8_t *) ev, sizeof(*ev));

	clock = evclock(stream, ev);

	delta = clock - stream->lastclock;

	dbg(">>> %d.%d.%d %c %c %c % 20ld % 15ld ",
			stream->loom, stream->proc, stream->tid,
			ev->header.model, ev->header.category, ev->header.value, clock, delta);

	payloadsize = ovni_payload_size(ev);
	for(i=0; i<payloadsize; i++)
	{
		dbg("%d ", ev->payload.u8[i]);
	}
	dbg("\n");
}

static void
print_cur_ev(struct ovni_emu *emu)
{
	print_ev(emu->cur_stream, emu->cur_ev);
}

/* Update the tracking channel if needed */
static void
cpu_update_tracking_chan(struct ovni_chan *cpu_chan, struct ovni_ethread *th)
{
	int th_enabled, cpu_enabled, st;
	struct ovni_chan *th_chan;

	assert(th);

	switch (cpu_chan->track)
	{
		case CHAN_TRACK_TH_RUNNING:
			cpu_enabled = th->is_running;
			break;
		case CHAN_TRACK_TH_ACTIVE:
			cpu_enabled = th->is_active;
			break;
		default:
			dbg("ignoring thread %d chan %d with track=%d\n",
				th->tid, cpu_chan->id, cpu_chan->track);
			return;
	}

	th_chan = &th->chan[cpu_chan->id];
	th_enabled = chan_is_enabled(th_chan);

	/* Enable the cpu channel if needed */
	if(cpu_enabled && !chan_is_enabled(cpu_chan))
		chan_enable(cpu_chan, cpu_enabled);

	/* Copy the state from the thread channel if needed */
	if(th_enabled && cpu_enabled)
	{
		/* Both enabled: simply follow the same value */
		st = chan_get_st(th_chan);
		if(chan_get_st(cpu_chan) != st)
			chan_set(cpu_chan, st);
	}
	else if(th_enabled && !cpu_enabled)
	{
		/* Only thread enabled: disable CPU */
		if(chan_is_enabled(cpu_chan))
			chan_disable(cpu_chan);
	}
	else if(!th_enabled && cpu_enabled)
	{
		/* Only CPU enabled: is this possible? Set to bad */
		chan_set(cpu_chan, ST_BAD);
		err("warning: cpu %s chan %d enabled but tracked thread %d chan disabled\n",
				cpu_chan->cpu->name,
				cpu_chan->id,
				th->tid);
	}
	else
	{
		/* Both disabled: disable CPU channel if needed */
		if(chan_is_enabled(cpu_chan))
			chan_disable(cpu_chan);
	}

	dbg("cpu %s chan %d enabled=%d state=%d\n",
			cpu_chan->cpu->name, cpu_chan->id,
			chan_is_enabled(cpu_chan),
			chan_get_st(cpu_chan));

}

void
emu_cpu_update_chan(struct ovni_cpu *cpu, struct ovni_chan *cpu_chan)
{
	int count;
	struct ovni_ethread *th;

	/* Determine the source of tracking */

	switch (cpu_chan->track)
	{
		case CHAN_TRACK_TH_RUNNING:
			count = cpu->nrunning_threads;
			th = cpu->th_running;
			break;
		case CHAN_TRACK_TH_ACTIVE:
			count = cpu->nactive_threads;
			th = cpu->th_active;
			break;
		default:
			dbg("ignoring %s chan %d with track=%d\n",
				cpu->name, cpu_chan->id, cpu_chan->track);
			return;
	}

	/* Based on how many threads, determine the state */
	if(count == 0)
	{
		/* The channel can be already disabled (migration of paused
		 * thread) so only disable it if needed. */
		if(chan_is_enabled(cpu_chan))
			chan_disable(cpu_chan);
	}
	else if(count == 1)
	{
		assert(th);

		/* A unique thread found: copy the state */
		dbg("cpu_update_chan: unique thread %d found, updating chan %d\n",
				th->tid, cpu_chan->id);

		cpu_update_tracking_chan(cpu_chan, th);
	}
	else
	{
		/* More than one thread: enable the channel and set it to a
		 * error value */
		if(!chan_is_enabled(cpu_chan))
			chan_enable(cpu_chan, 1);

		if(chan_get_st(cpu_chan) != ST_TOO_MANY_TH)
			chan_set(cpu_chan, ST_TOO_MANY_TH);
	}
}

static void
propagate_channels(struct ovni_emu *emu)
{
	struct ovni_cpu *cpu;
	struct ovni_chan *cpu_chan, *th_chan, *tmp;
	struct ovni_ethread *thread;

	/* Only propagate thread channels to their corresponding CPU */

	DL_FOREACH_SAFE(emu->th_chan, th_chan, tmp)
	{
		assert(th_chan->thread);

		thread = th_chan->thread;

		/* No CPU in the thread */
		if(thread->cpu == NULL)
			continue;

		cpu = thread->cpu;

		cpu_chan = &cpu->chan[th_chan->id];

		dbg("propagate thread %d chan %d in cpu %s\n",
				thread->tid, th_chan->id, cpu->name);

		emu_cpu_update_chan(cpu, cpu_chan);
	}
}

static void
emit_channels(struct ovni_emu *emu)
{
	struct ovni_chan *cpu_chan, *th_chan;

	dbg("emu enters emit_channels -------------------\n");

	/* Emit only the channels that have been updated. We need a list
	 * of updated channels */
	DL_FOREACH(emu->th_chan, th_chan)
	{
		dbg("emu emits th chan %d\n", th_chan->id);
		//assert(th_chan->dirty);
		chan_emit(th_chan);
	}

	DL_FOREACH(emu->cpu_chan, cpu_chan)
	{
		dbg("emu emits cpu chan %d\n", cpu_chan->id);
		//assert(cpu_chan->dirty);
		chan_emit(cpu_chan);
	}

	/* Reset the list of updated channels */
	dbg("emu resets the list of dirty channels -------------------\n");
	emu->th_chan = NULL;
	emu->cpu_chan = NULL;
}

static void
hook_init(struct ovni_emu *emu)
{
	hook_init_ovni(emu);
	hook_init_nosv(emu);
	hook_init_tampi(emu);
	hook_init_openmp(emu);
	hook_init_nanos6(emu);
}

static void
hook_pre(struct ovni_emu *emu)
{
	switch(emu->cur_ev->header.model)
	{
		case 'O': hook_pre_ovni(emu); break;
		case 'V': hook_pre_nosv(emu); break;
		case 'T': hook_pre_tampi(emu); break;
		case 'M': hook_pre_openmp(emu); break;
		case 'L': hook_pre_nanos6(emu); break;
		default:
			break;
	}
}

static void
hook_post(struct ovni_emu *emu)
{
	switch(emu->cur_ev->header.model)
	{
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

/*
 * heap_node_compare_t - comparison function, returns:

 * 	> 0 if a > b
 * 	< 0 if a < b
 * 	= 0 if a == b
 *
 * Invert the comparison function to get a min-heap instead
 */
static inline int stream_cmp(heap_node_t *a, heap_node_t *b)
{
	struct ovni_stream *sa, *sb;

	sa = heap_elem(a, struct ovni_stream, hh);
	sb = heap_elem(b, struct ovni_stream, hh);

	if(sb->lastclock < sa->lastclock)
		return -1;
	else if(sb->lastclock > sa->lastclock)
		return +1;
	else
		return 0;
}

static double
get_time(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec * 1.0e-9;
}

static void
print_progress(struct ovni_emu *emu)
{
	double progress, time_now, time_elapsed, time_total, time_left;
	double speed_in, speed_out;
	double cpu_written, th_written, written;
	int min, sec;

	cpu_written = (double) ftell(emu->prv_cpu);
	th_written = (double) ftell(emu->prv_thread);

	written = cpu_written + th_written;

	progress = ((double) emu->global_offset) /
		((double) emu->global_size);

	time_now = get_time();
	time_elapsed = time_now - emu->start_emulation_time;
	time_total = time_elapsed / progress;
	time_left = time_total - time_elapsed;

	speed_in = (double) emu->nev_processed / time_elapsed;
	speed_out = written / time_elapsed;

	min = (int) (time_left / 60.0);
	sec = (int) ((time_left / 60.0 - min) * 60);

	err("%.1f%% done at %.0f Kev/s, out %.1f GB CPU / %.1f GB TH at %.1f MB/s (%d min %d s left)\n",
			100.0 * progress,
			speed_in / 1024.0,
			cpu_written / (1024.0 * 1024.0 * 1024.0),
			th_written / (1024.0 * 1024.0 * 1024.0),
			speed_out / (1024.0 * 1024),
			min, sec);
}

/* Loads the next event and sets the lastclock in the stream.
 * Returns -1 if the stream has no more events. */
static int
emu_step_stream(struct ovni_emu *emu, struct ovni_stream *stream)
{
	if(ovni_load_next_event(stream) < 0)
		return -1;

	stream->lastclock = evclock(stream, stream->cur_ev);

	heap_insert(&emu->sorted_stream, &stream->hh, &stream_cmp);

	return 0;
}

static int
next_event(struct ovni_emu *emu)
{
	struct ovni_stream *stream;
	heap_node_t *node;

	static int done_first = 0;

	/* Extract the next stream based on the event clock */
	node = heap_pop_max(&emu->sorted_stream, stream_cmp);

	/* No more streams */
	if(node == NULL)
		return -1;

	stream = heap_elem(node, struct ovni_stream, hh);

	assert(stream);

	set_current(emu, stream);

	emu->global_offset += ovni_ev_size(stream->cur_ev);

	//err("stream %d clock at %ld\n", stream->tid, stream->lastclock);

	/* This can happen if two events are not ordered in the stream, but the
	 * emulator picks other events in the middle. Example:
	 *
	 * Stream A: 10 3 ...
	 * Stream B: 5 12
	 *
	 * emulator output:
	 * 5
	 * 10
	 * 3  -> warning!
	 * 12
	 * ...
	 * */
	if(emu->lastclock > stream->lastclock)
	{
		err("warning: backwards jump in time %lu -> %lu for tid %d\n",
				emu->lastclock, stream->lastclock, stream->tid);

		if(emu->enable_linter)
			exit(EXIT_FAILURE);
	}

	emu->lastclock = stream->lastclock;

	if(!done_first)
	{
		done_first = 1;
		emu->firstclock = emu->lastclock;
	}

	emu->delta_time = emu->lastclock - emu->firstclock;

	return 0;
}

static void
emu_load_first_events(struct ovni_emu *emu)
{
	size_t i;
	struct ovni_trace *trace;
	struct ovni_stream *stream;

	/* Prepare the stream heap */
	heap_init(&emu->sorted_stream);

	emu->lastclock = 0;

	/* Load initial streams and events */
	trace = &emu->trace;
	for(i=0; i<trace->nstreams; i++)
	{
		stream = &trace->stream[i];
		emu->global_size += stream->size;

		if(emu_step_stream(emu, stream) < 0)
		{
			dbg("warning: empty stream for tid %d\n", stream->tid);
			continue;
		}

		assert(stream->active);
	}
}

static void
emulate(struct ovni_emu *emu)
{
	size_t i;

	emu->nev_processed = 0;

	err("loading first events\n");
	emu_load_first_events(emu);

	err("emulation starts\n");
	emu->start_emulation_time = get_time();

	hook_init(emu);

	emit_channels(emu);

	/* Then process all events */
	for(i=0; next_event(emu) == 0; i++)
	{
		print_cur_ev(emu);

		hook_pre(emu);

		propagate_channels(emu);
		emit_channels(emu);

		hook_post(emu);

		if(i >= 50000)
		{
			print_progress(emu);
			i = 0;
		}

		emu->nev_processed++;

		emu_step_stream(emu, emu->cur_stream);
	}

	print_progress(emu);
}

struct ovni_ethread *
emu_get_thread(struct ovni_eproc *proc, int tid)
{
	size_t i;
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
add_new_cpu(struct ovni_emu *emu, struct ovni_loom *loom, int i, int phyid)
{
	struct ovni_cpu *cpu;

	/* The logical id must match our index */
	assert((size_t) i == loom->ncpus);

	cpu = &loom->cpu[i];

	assert(cpu->state == CPU_ST_UNKNOWN);

	cpu->state = CPU_ST_READY;
	cpu->i = i;
	cpu->phyid = phyid;
	cpu->gindex = emu->total_ncpus++;
	cpu->loom = loom;

	dbg("new cpu %d at phyid=%d\n", cpu->gindex, phyid);

	loom->ncpus++;
}

static void
proc_load_cpus(struct ovni_emu *emu, struct ovni_loom *loom, struct ovni_eproc *proc)
{
	JSON_Array *cpuarray;
	JSON_Object *cpu;
	JSON_Object *meta;
	size_t i, index, phyid;
	struct ovni_cpu *vcpu;

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

	/* Init the vcpu as well */
	vcpu = &loom->vcpu;
	assert(vcpu->state == CPU_ST_UNKNOWN);

	vcpu->state = CPU_ST_READY;
	vcpu->i = -1;
	vcpu->phyid = -1;
	vcpu->gindex = emu->total_ncpus++;
	vcpu->loom = loom;

	dbg("new vcpu %d\n", vcpu->gindex);
}

/* Obtain CPUs in the metadata files and other data */
static void
load_metadata(struct ovni_emu *emu)
{
	size_t i, j;
	struct ovni_loom *loom;
	struct ovni_eproc *proc;
	struct ovni_trace *trace;

	trace = &emu->trace;

	for(i=0; i<trace->nlooms; i++)
	{
		loom = &trace->loom[i];
		loom->offset_ncpus = emu->total_ncpus;

		for(j=0; j<loom->nprocs; j++)
		{
			proc = &loom->proc[j];

			proc_load_cpus(emu, loom, proc);
		}

		/* One of the process must have the list of CPUs */
		/* FIXME: The CPU list should be at the loom dir */
		assert(loom->ncpus > 0);
	}
}

static int
destroy_metadata(struct ovni_emu *emu)
{
	size_t i, j;
	struct ovni_loom *loom;
	struct ovni_eproc *proc;
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
	{
		err("error opening thread PRV file %s: %s\n", path,
				strerror(errno));
		exit(EXIT_FAILURE);
	}

	sprintf(path, "%s/%s", tracedir, "cpu.prv");

	emu->prv_cpu = fopen(path, "w");

	if(emu->prv_cpu == NULL)
	{
		err("error opening cpu PRV file %s: %s\n", path,
				strerror(errno));
		exit(EXIT_FAILURE);
	}

	prv_header(emu->prv_thread, emu->total_nthreads);
	prv_header(emu->prv_cpu, emu->total_ncpus);
}

static void
open_pcfs(struct ovni_emu *emu, char *tracedir)
{
	char path[PATH_MAX];

	sprintf(path, "%s/%s", tracedir, "thread.pcf");

	emu->pcf_thread = fopen(path, "w");

	if(emu->pcf_thread == NULL)
	{
		err("error opening thread PCF file %s: %s\n", path,
				strerror(errno));
		exit(EXIT_FAILURE);
	}

	sprintf(path, "%s/%s", tracedir, "cpu.pcf");

	emu->pcf_cpu = fopen(path, "w");

	if(emu->pcf_cpu == NULL)
	{
		err("error opening cpu PCF file %s: %s\n", path,
				strerror(errno));
		exit(EXIT_FAILURE);
	}
}

/* Fix the trace duration at the end */
static void
fix_prv_headers(struct ovni_emu *emu)
{
	prv_fix_header(emu->prv_thread, emu->delta_time, emu->total_nthreads);
	prv_fix_header(emu->prv_cpu, emu->delta_time, emu->total_ncpus);
}

static void
close_prvs(struct ovni_emu *emu)
{
	fclose(emu->prv_thread);
	fclose(emu->prv_cpu);
}

static void
close_pcfs(struct ovni_emu *emu)
{
	fclose(emu->pcf_thread);
	fclose(emu->pcf_cpu);
}

static void
usage(int argc, char *argv[])
{
	UNUSED(argc);
	UNUSED(argv);

	err("Usage: emu [-c offsetfile] tracedir\n");
	err("\n");
	err("Options:\n");
	err("  -c offsetfile      Use the given offset file to correct\n");
	err("                     the clocks among nodes. It can be\n");
	err("                     generated by the ovnisync program\n");
	err("\n");
	err("  tracedir           The output trace dir generated by ovni.\n");
	err("\n");
	err("The output PRV files are placed in the tracedir directory.\n");

	exit(EXIT_FAILURE);
}

static void
parse_args(struct ovni_emu *emu, int argc, char *argv[])
{
	int opt;

	while((opt = getopt(argc, argv, "c:l")) != -1)
	{
		switch(opt)
		{
			case 'c':
				emu->clock_offset_file = optarg;
				break;
			case 'l':
				emu->enable_linter = 1;
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

	emu->tracedir = argv[optind];
}

static struct ovni_loom *
find_loom_by_hostname(struct ovni_emu *emu, char *host)
{
	size_t i;
	struct ovni_loom *loom;

	for(i=0; i<emu->trace.nlooms; i++)
	{
		loom = &emu->trace.loom[i];

		if(strcmp(loom->hostname, host) == 0)
			return loom;
	}

	return NULL;
}

static void
load_clock_offsets(struct ovni_emu *emu)
{
	FILE *f;
	char buf[1024];
	size_t i;
	int rank, ret;
	double offset, mean, std;
	char host[OVNI_MAX_HOSTNAME];
	struct ovni_loom *loom;
	struct ovni_trace *trace;
	struct ovni_stream *stream;

	f = fopen(emu->clock_offset_file, "r");

	if(f == NULL)
	{
		err("error opening clock offset file %s: %s\n",
				emu->clock_offset_file,
				strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Ignore header line */
	if(fgets(buf, 1024, f) == NULL)
	{
		perror("fgets failed");
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		errno = 0;
		ret = fscanf(f, "%d %s %lf %lf %lf", &rank, host, &offset, &mean, &std);

		if(ret == EOF)
		{
			if(errno != 0)
			{
				perror("fscanf failed");
				exit(EXIT_FAILURE);
			}

			break;
		}

		if(ret != 5)
		{
			err("fscanf read %d instead of 5 fields in %s\n",
					ret, emu->clock_offset_file);
			exit(EXIT_FAILURE);
		}

		loom = find_loom_by_hostname(emu, host);

		if(loom == NULL)
		{
			err("No loom has hostname %s\n", host);
			exit(EXIT_FAILURE);
		}

		if(loom->clock_offset != 0)
		{
			err("warning: loom at host %s already has a clock offset\n",
					host);
		}

		loom->clock_offset = (int64_t) offset;
	}

	/* Then populate the stream offsets */

	trace = &emu->trace;

	for(i=0; i<trace->nstreams; i++)
	{
		stream = &trace->stream[i];
		loom = &trace->loom[stream->loom];
		stream->clock_offset = loom->clock_offset;
	}

	fclose(f);
}

static void
write_row_cpu(struct ovni_emu *emu)
{
	FILE *f;
	size_t i;
	char path[PATH_MAX];
	struct ovni_cpu *cpu;

	sprintf(path, "%s/%s", emu->tracedir, "cpu.row");

	f = fopen(path, "w");

	if(f == NULL)
	{
		perror("cannot open row file");
		exit(EXIT_FAILURE);
	}

	fprintf(f, "LEVEL NODE SIZE 1\n");
	fprintf(f, "hostname\n");
	fprintf(f, "\n");

	fprintf(f, "LEVEL THREAD SIZE %ld\n", emu->total_ncpus);

	for(i=0; i<emu->total_ncpus; i++)
	{
		cpu = emu->global_cpu[i];
		fprintf(f, "%s\n", cpu->name);
	}

	fclose(f);
}

static void
write_row_thread(struct ovni_emu *emu)
{
	FILE *f;
	size_t i;
	char path[PATH_MAX];
	struct ovni_ethread *th;

	sprintf(path, "%s/%s", emu->tracedir, "thread.row");

	f = fopen(path, "w");

	if(f == NULL)
	{
		perror("cannot open row file");
		exit(EXIT_FAILURE);
	}

	fprintf(f, "LEVEL NODE SIZE 1\n");
	fprintf(f, "hostname\n");
	fprintf(f, "\n");

	fprintf(f, "LEVEL THREAD SIZE %ld\n", emu->total_nthreads);

	for(i=0; i<emu->total_nthreads; i++)
	{
		th = emu->global_thread[i];
		fprintf(f, "THREAD %d.%d\n", th->proc->appid, th->tid);
	}

	fclose(f);
}

static void
init_threads(struct ovni_emu *emu)
{
	struct ovni_trace *trace;
	struct ovni_loom *loom;
	struct ovni_eproc *proc;
	struct ovni_ethread *thread;
	size_t i, j, k, gi;

	emu->total_nthreads = 0;

	trace = &emu->trace;

	for(i=0; i<trace->nlooms; i++)
	{
		loom = &trace->loom[i];
		for(j=0; j<loom->nprocs; j++)
		{
			proc = &loom->proc[j];
			for(k=0; k<proc->nthreads; k++)
			{
				thread = &proc->thread[k];

				emu->total_nthreads++;
			}
		}
	}

	emu->global_thread = calloc(emu->total_nthreads,
			sizeof(*emu->global_thread));

	if(emu->global_thread == NULL)
	{
		perror("calloc failed");
		exit(EXIT_FAILURE);
	}

	for(gi=0, i=0; i<trace->nlooms; i++)
	{
		loom = &trace->loom[i];
		for(j=0; j<loom->nprocs; j++)
		{
			proc = &loom->proc[j];
			for(k=0; k<proc->nthreads; k++)
			{
				thread = &proc->thread[k];

				emu->global_thread[gi++] = thread;
			}
		}
	}
}

static void
init_cpus(struct ovni_emu *emu)
{
	struct ovni_trace *trace;
	struct ovni_loom *loom;
	struct ovni_cpu *cpu;
	size_t i, j;

	trace = &emu->trace;

	emu->global_cpu = calloc(emu->total_ncpus,
			sizeof(*emu->global_cpu));

	if(emu->global_cpu == NULL)
	{
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	for(i=0; i<trace->nlooms; i++)
	{
		loom = &trace->loom[i];
		for(j=0; j<loom->ncpus; j++)
		{
			cpu = &loom->cpu[j];
			emu->global_cpu[cpu->gindex] = cpu;

			if(snprintf(cpu->name, MAX_CPU_NAME, "CPU %ld.%ld",
						i, j) >= MAX_CPU_NAME)
			{
				err("error cpu %ld.%ld name too long\n", i, j);
				exit(EXIT_FAILURE);
			}
		}

		emu->global_cpu[loom->vcpu.gindex] = &loom->vcpu;
		if(snprintf(loom->vcpu.name, MAX_CPU_NAME, "CPU %ld.*",
					i) >= MAX_CPU_NAME)
		{
			err("error cpu %ld.* name too long\n", i);
			exit(EXIT_FAILURE);
		}
	}
}

static void
emu_init(struct ovni_emu *emu, int argc, char *argv[])
{
	memset(emu, 0, sizeof(*emu));

	parse_args(emu, argc, argv);

	if(ovni_load_trace(&emu->trace, emu->tracedir))
	{
		err("error loading ovni trace\n");
		exit(EXIT_FAILURE);
	}

	if(ovni_load_streams(&emu->trace))
	{
		err("error loading streams\n");
		exit(EXIT_FAILURE);
	}

	load_metadata(emu);

	if(emu->clock_offset_file != NULL)
		load_clock_offsets(emu);

	init_threads(emu);
	init_cpus(emu);

	open_prvs(emu, emu->tracedir);
	open_pcfs(emu, emu->tracedir);

	emu->global_size = 0;
	emu->global_offset = 0;

	err("loaded %ld cpus and %ld threads\n",
			emu->total_ncpus,
			emu->total_nthreads);
}

static void
emu_post(struct ovni_emu *emu)
{
	/* Write the PCF files */
	pcf_write(emu->pcf_thread, emu);
	pcf_write(emu->pcf_cpu, emu);

	write_row_cpu(emu);
	write_row_thread(emu);
}

static void
emu_destroy(struct ovni_emu *emu)
{
	fix_prv_headers(emu);
	close_prvs(emu);
	close_pcfs(emu);
	destroy_metadata(emu);
	ovni_free_streams(&emu->trace);
	ovni_free_trace(&emu->trace);

	free(emu->global_cpu);
	free(emu->global_thread);
}

int
main(int argc, char *argv[])
{
	struct ovni_emu *emu;

	emu = malloc(sizeof(struct ovni_emu));

	if(emu == NULL)
	{
		perror("malloc");
		return 1;
	}

	emu_init(emu, argc, argv);

	emulate(emu);

	emu_post(emu);

	emu_destroy(emu);

	free(emu);

	return 0;
}
