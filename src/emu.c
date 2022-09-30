/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 200112L

#include <dirent.h>
#include <errno.h>
#include <linux/limits.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "chan.h"
#include "emu.h"
#include "ovni.h"
#include "pcf.h"
#include "prv.h"
#include "trace.h"
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
	int64_t clock = evclock(stream, ev);

	int64_t delta = clock - stream->lastclock;
	UNUSED(delta);

	dbg(">>> %s.%d.%d %c %c %c % 20ld % 15ld ",
			stream->loom->hostname,
			stream->proc->pid,
			stream->thread->tid,
			ev->header.model, ev->header.category, ev->header.value, clock, delta);

	int payloadsize = ovni_payload_size(ev);
	for (int i = 0; i < payloadsize; i++) {
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
	int cpu_enabled = 0;

	switch (cpu_chan->track) {
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

	struct ovni_chan *th_chan = &th->chan[cpu_chan->id];
	int th_enabled = chan_is_enabled(th_chan);

	/* Enable the cpu channel if needed */
	if (cpu_enabled && !chan_is_enabled(cpu_chan))
		chan_enable(cpu_chan, cpu_enabled);

	/* Copy the state from the thread channel if needed */
	if (th_enabled && cpu_enabled) {
		/* Both enabled: simply follow the same value */
		int st = chan_get_st(th_chan);
		if (chan_get_st(cpu_chan) != st)
			chan_set(cpu_chan, st);
	} else if (th_enabled && !cpu_enabled) {
		/* Only thread enabled: disable CPU */
		if (chan_is_enabled(cpu_chan))
			chan_disable(cpu_chan);
	} else if (!th_enabled && cpu_enabled) {
		/* Only CPU enabled: is this possible? Set to bad */
		chan_set(cpu_chan, ST_BAD);
		err("warning: cpu %s chan %d enabled but tracked thread %d chan disabled\n",
				cpu_chan->cpu->name,
				cpu_chan->id,
				th->tid);
	} else {
		/* Both disabled: disable CPU channel if needed */
		if (chan_is_enabled(cpu_chan))
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
	int count = 0;
	struct ovni_ethread *th = NULL;

	/* Determine the source of tracking */

	switch (cpu_chan->track) {
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
	if (count == 0) {
		/* The channel can be already disabled (migration of paused
		 * thread) so only disable it if needed. */
		if (chan_is_enabled(cpu_chan))
			chan_disable(cpu_chan);
	} else if (count == 1) {
		if (th == NULL)
			die("emu_cpu_update_chan: tracking thread is NULL\n");

		/* A unique thread found: copy the state */
		dbg("cpu_update_chan: unique thread %d found, updating chan %d\n",
				th->tid, cpu_chan->id);

		cpu_update_tracking_chan(cpu_chan, th);
	} else {
		/* More than one thread: enable the channel and set it to a
		 * error value */
		if (!chan_is_enabled(cpu_chan))
			chan_enable(cpu_chan, 1);

		if (chan_get_st(cpu_chan) != ST_TOO_MANY_TH)
			chan_set(cpu_chan, ST_TOO_MANY_TH);
	}
}

static void
propagate_channels(struct ovni_emu *emu)
{
	struct ovni_chan *th_chan = NULL;
	struct ovni_chan *tmp = NULL;

	/* Only propagate thread channels to their corresponding CPU */

	DL_FOREACH_SAFE(emu->th_chan, th_chan, tmp)
	{
		if (th_chan->thread == NULL)
			die("propagate_channels: channel thread is NULL\n");

		struct ovni_ethread *thread = th_chan->thread;

		/* No CPU in the thread */
		if (thread->cpu == NULL)
			continue;

		struct ovni_cpu *cpu = thread->cpu;

		struct ovni_chan *cpu_chan = &cpu->chan[th_chan->id];

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
		chan_emit(th_chan);
	}

	DL_FOREACH(emu->cpu_chan, cpu_chan)
	{
		dbg("emu emits cpu chan %d\n", cpu_chan->id);
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
	hook_init_nodes(emu);
	hook_init_kernel(emu);
	hook_init_nanos6(emu);
}

static void
hook_end(struct ovni_emu *emu)
{
	hook_end_nosv(emu);
	hook_end_nanos6(emu);
}

static void
hook_pre(struct ovni_emu *emu)
{
	switch (emu->cur_ev->header.model) {
		case 'O':
			hook_pre_ovni(emu);
			break;
		case 'V':
			hook_pre_nosv(emu);
			break;
		case 'T':
			hook_pre_tampi(emu);
			break;
		case 'M':
			hook_pre_openmp(emu);
			break;
		case 'D':
			hook_pre_nodes(emu);
			break;
		case 'K':
			hook_pre_kernel(emu);
			break;
		case '6':
			hook_pre_nanos6(emu);
			break;
		default:
			break;
	}
}

static void
hook_post(struct ovni_emu *emu)
{
	switch (emu->cur_ev->header.model) {
		default:
			// dbg("unknown model %c\n", emu->cur_ev->model);
			break;
	}
}

static void
set_current(struct ovni_emu *emu, struct ovni_stream *stream)
{
	emu->cur_stream = stream;
	emu->cur_ev = stream->cur_ev;
	emu->cur_loom = stream->loom;
	emu->cur_proc = stream->proc;
	emu->cur_thread = stream->thread;
}

/*
 * heap_node_compare_t - comparison function, returns:

 * 	> 0 if a > b
 * 	< 0 if a < b
 * 	= 0 if a == b
 *
 * Invert the comparison function to get a min-heap instead
 */
static inline int
stream_cmp(heap_node_t *a, heap_node_t *b)
{
	struct ovni_stream *sa, *sb;

	sa = heap_elem(a, struct ovni_stream, hh);
	sb = heap_elem(b, struct ovni_stream, hh);

	if (sb->lastclock < sa->lastclock)
		return -1;
	else if (sb->lastclock > sa->lastclock)
		return +1;
	else
		return 0;
}

static double
get_time(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + (double) ts.tv_nsec * 1.0e-9;
}

static void
print_progress(struct ovni_emu *emu)
{
	double cpu_written = (double) ftell(emu->prv_cpu);
	double th_written = (double) ftell(emu->prv_thread);

	double written = cpu_written + th_written;

	double progress = ((double) emu->global_offset)
			  / ((double) emu->global_size);

	double time_now = get_time();
	double time_elapsed = time_now - emu->start_emulation_time;
	double time_total = time_elapsed / progress;
	double time_left = time_total - time_elapsed;

	double speed_in = (double) emu->nev_processed / time_elapsed;
	double speed_out = written / time_elapsed;

	int tmin = (int) (time_left / 60.0);
	int sec = (int) ((time_left / 60.0 - tmin) * 60);

	err("%.1f%% done at %.0f Kev/s, out %.1f GB CPU / %.1f GB TH at %.1f MB/s (%d min %d s left)\n",
			100.0 * progress,
			speed_in / 1024.0,
			cpu_written / (1024.0 * 1024.0 * 1024.0),
			th_written / (1024.0 * 1024.0 * 1024.0),
			speed_out / (1024.0 * 1024),
			tmin, sec);
}

/* Loads the next event and sets the lastclock in the stream.
 * Returns -1 if the stream has no more events. */
static int
emu_step_stream(struct ovni_emu *emu, struct ovni_stream *stream)
{
	if (ovni_load_next_event(stream) < 0)
		return -1;

	stream->lastclock = evclock(stream, stream->cur_ev);

	heap_insert(&emu->sorted_stream, &stream->hh, &stream_cmp);

	return 0;
}

static int
next_event(struct ovni_emu *emu)
{
	static int done_first = 0;

	/* Extract the next stream based on the event clock */
	heap_node_t *node = heap_pop_max(&emu->sorted_stream, stream_cmp);

	/* No more streams */
	if (node == NULL)
		return -1;

	struct ovni_stream *stream = heap_elem(node, struct ovni_stream, hh);

	if (stream == NULL)
		die("next_event: heap_elem returned NULL\n");

	set_current(emu, stream);

	emu->global_offset += ovni_ev_size(stream->cur_ev);

	// err("stream %d clock at %ld\n", stream->tid, stream->lastclock);

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
	if (emu->lastclock > stream->lastclock) {
		err("warning: backwards jump in time %lu -> %lu for tid %d\n",
				emu->lastclock, stream->lastclock, stream->tid);

		if (emu->enable_linter)
			abort();
	}

	emu->lastclock = stream->lastclock;

	if (!done_first) {
		done_first = 1;
		emu->firstclock = emu->lastclock;
	}

	emu->delta_time = emu->lastclock - emu->firstclock;

	return 0;
}

static void
emu_load_first_events(struct ovni_emu *emu)
{
	/* Prepare the stream heap */
	heap_init(&emu->sorted_stream);

	emu->lastclock = 0;

	/* Load initial streams and events */
	struct ovni_trace *trace = &emu->trace;
	for (size_t i = 0; i < trace->nstreams; i++) {
		struct ovni_stream *stream = &trace->stream[i];
		emu->global_size += stream->size;

		if (emu_step_stream(emu, stream) < 0) {
			err("warning: empty stream for tid %d\n", stream->tid);

			if (emu->enable_linter)
				abort();

			continue;
		}
	}
}

static void
emulate(struct ovni_emu *emu)
{
	emu->nev_processed = 0;

	err("loading first events\n");
	emu_load_first_events(emu);

	err("emulation starts\n");
	emu->start_emulation_time = get_time();

	hook_init(emu);

	emit_channels(emu);

	/* Then process all events */
	for (size_t i = 0; next_event(emu) == 0; i++) {
		print_cur_ev(emu);

		hook_pre(emu);

		propagate_channels(emu);
		emit_channels(emu);

		hook_post(emu);

		if (i >= 50000) {
			print_progress(emu);
			i = 0;
		}

		emu->nev_processed++;

		emu_step_stream(emu, emu->cur_stream);
	}

	hook_end(emu);

	print_progress(emu);
}

struct ovni_ethread *
emu_get_thread(struct ovni_eproc *proc, int tid)
{
	for (size_t i = 0; i < proc->nthreads; i++) {
		struct ovni_ethread *thread = &proc->thread[i];
		if (thread->tid == tid)
			return thread;
	}

	return NULL;
}

static void
add_new_cpu(struct ovni_emu *emu, struct ovni_loom *loom, int i, int phyid)
{
	struct ovni_cpu *cpu = &loom->cpu[i];

	if (i < 0 || i >= (int) loom->ncpus)
		die("CPU with index %d in loom %s is out of bounds\n",
				i, loom->hostname);

	if (cpu->state != CPU_ST_UNKNOWN)
		die("new cpu %d in unexpected state in loom %s\n",
				i, loom->hostname);

	cpu->state = CPU_ST_READY;
	cpu->i = i;
	cpu->phyid = phyid;
	cpu->gindex = emu->total_ncpus++;
	cpu->loom = loom;

	dbg("new cpu %d at phyid=%d\n", cpu->gindex, phyid);
}

static int
proc_load_cpus(struct ovni_emu *emu, struct ovni_loom *loom,
		struct ovni_eproc *proc,
		struct ovni_eproc *metadata_proc)
{
	JSON_Object *meta = json_value_get_object(proc->meta);
	if (meta == NULL)
		die("json_value_get_object() failed\n");

	JSON_Array *cpuarray = json_object_get_array(meta, "cpus");

	/* This process doesn't have the cpu list */
	if (cpuarray == NULL)
		return -1;

	if (metadata_proc)
		die("duplicated metadata for proc %d and %d in loom %s\n",
				metadata_proc->pid, proc->pid,
				loom->hostname);

	if (loom->ncpus != 0)
		die("loom %s already has CPUs\n", loom->hostname);

	loom->ncpus = json_array_get_count(cpuarray);

	if (loom->ncpus == 0)
		die("loom %s proc %d has metadata but no CPUs\n",
				loom->hostname, proc->pid);

	loom->cpu = calloc(loom->ncpus, sizeof(struct ovni_cpu));

	if (loom->cpu == NULL)
		die("calloc failed: %s\n", strerror(errno));

	for (size_t i = 0; i < loom->ncpus; i++) {
		JSON_Object *cpu = json_array_get_object(cpuarray, i);

		if (cpu == NULL)
			die("proc_load_cpus: json_array_get_object() failed\n");

		int index = (int) json_object_get_number(cpu, "index");
		int phyid = (int) json_object_get_number(cpu, "phyid");

		add_new_cpu(emu, loom, index, phyid);
	}

	/* If we reach this point, all CPUs are in the ready state */

	/* Init the vcpu as well */
	struct ovni_cpu *vcpu = &loom->vcpu;
	if (vcpu->state != CPU_ST_UNKNOWN)
		die("unexpected virtual CPU state in loom %s\n",
				loom->hostname);

	vcpu->state = CPU_ST_READY;
	vcpu->i = -1;
	vcpu->phyid = -1;
	vcpu->gindex = emu->total_ncpus++;
	vcpu->loom = loom;

	dbg("new vcpu %d\n", vcpu->gindex);

	return 0;
}

/* Obtain CPUs in the metadata files and other data */
static void
load_metadata(struct ovni_emu *emu)
{
	struct ovni_trace *trace = &emu->trace;

	for (size_t i = 0; i < trace->nlooms; i++) {
		struct ovni_loom *loom = &trace->loom[i];
		loom->offset_ncpus = emu->total_ncpus;

		struct ovni_eproc *metadata_proc = NULL;

		for (size_t j = 0; j < loom->nprocs; j++) {
			struct ovni_eproc *proc = &loom->proc[j];

			if (proc_load_cpus(emu, loom, proc, metadata_proc) < 0)
				continue;

			if (metadata_proc)
				die("duplicated metadata found in pid %d and %d\n",
						metadata_proc->pid,
						proc->pid);

			metadata_proc = proc;
		}

		/* One of the process must have the list of CPUs */
		if (metadata_proc == NULL)
			die("no metadata found in loom %s\n", loom->hostname);

		if (loom->ncpus == 0)
			die("no CPUs found in loom %s\n", loom->hostname);
	}
}

static int
destroy_metadata(struct ovni_emu *emu)
{
	struct ovni_trace *trace = &emu->trace;

	for (size_t i = 0; i < trace->nlooms; i++) {
		struct ovni_loom *loom = &trace->loom[i];
		for (size_t j = 0; j < loom->nprocs; j++) {
			struct ovni_eproc *proc = &loom->proc[j];

			if (proc->meta == NULL)
				die("cannot destroy metadata: is NULL\n");

			json_value_free(proc->meta);
		}

		free(loom->cpu);
	}

	return 0;
}

static void
open_prvs(struct ovni_emu *emu, char *tracedir)
{
	char path[PATH_MAX];

	sprintf(path, "%s/%s", tracedir, "thread.prv");

	emu->prv_thread = fopen(path, "w");

	if (emu->prv_thread == NULL) {
		err("error opening thread PRV file %s: %s\n", path,
				strerror(errno));
		exit(EXIT_FAILURE);
	}

	sprintf(path, "%s/%s", tracedir, "cpu.prv");

	emu->prv_cpu = fopen(path, "w");

	if (emu->prv_cpu == NULL) {
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
	pcf_open(&emu->pcf[CHAN_TH], path, CHAN_TH);

	sprintf(path, "%s/%s", tracedir, "cpu.pcf");
	pcf_open(&emu->pcf[CHAN_CPU], path, CHAN_CPU);
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
	pcf_close(&emu->pcf[CHAN_TH]);
	pcf_close(&emu->pcf[CHAN_CPU]);
}

static void
usage(void)
{
	err("Usage: ovniemu [-c offsetfile] tracedir\n");
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

	while ((opt = getopt(argc, argv, "c:l")) != -1) {
		switch (opt) {
			case 'c':
				emu->clock_offset_file = optarg;
				break;
			case 'l':
				emu->enable_linter = 1;
				break;
			default: /* '?' */
				usage();
		}
	}

	if (optind >= argc) {
		err("missing tracedir\n");
		usage();
	}

	emu->tracedir = argv[optind];
}

static void
set_clock_offsets(struct ovni_emu *emu, const char *host, size_t offset)
{
	size_t matches = 0;

	for (size_t i = 0; i < emu->trace.nlooms; i++) {
		struct ovni_loom *loom = &emu->trace.loom[i];

		/* Match the hostname exactly */
		if (strcmp(loom->hostname, host) != 0)
			continue;

		if (loom->clock_offset != 0)
			die("loom %s already has a clock offset\n", loom->dname);

		loom->clock_offset = offset;
		matches++;
	}

	if (matches == 0)
		die("no loom has hostname %s\n", host);
}

static void
load_clock_offsets(struct ovni_emu *emu)
{
	FILE *f = NULL;

	if (emu->clock_offset_file != NULL) {
		f = fopen(emu->clock_offset_file, "r");

		/* If provided by the user, it must exist */
		if (f == NULL) {
			err("error opening clock offset file %s: %s\n",
					emu->clock_offset_file,
					strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		char path[PATH_MAX];
		if (snprintf(path, PATH_MAX, "%s/clock-offsets.txt",
				    emu->tracedir)
				>= PATH_MAX) {
			die("clock offset path too long\n");
		}

		f = fopen(path, "r");

		if (f == NULL) {
			/* May not exist, but is fine */
			return;
		}
	}

	/* Ignore header line */
	char buf[1024];
	if (fgets(buf, 1024, f) == NULL) {
		err("missing header line in clock offset file");
		exit(EXIT_FAILURE);
	}

	while (1) {
		errno = 0;
		int rank, ret;
		double offset, mean, std;
		char host[OVNI_MAX_HOSTNAME];
		ret = fscanf(f, "%d %s %lf %lf %lf", &rank, host, &offset, &mean, &std);

		if (ret == EOF) {
			if (errno != 0) {
				perror("fscanf failed");
				exit(EXIT_FAILURE);
			}

			break;
		}

		if (ret != 5) {
			err("fscanf read %d instead of 5 fields in %s\n",
					ret, emu->clock_offset_file);
			exit(EXIT_FAILURE);
		}

		set_clock_offsets(emu, host, (int64_t) offset);
	}

	/* Then populate the stream offsets */

	struct ovni_trace *trace = &emu->trace;

	for (size_t i = 0; i < trace->nstreams; i++) {
		struct ovni_stream *stream = &trace->stream[i];
		struct ovni_loom *loom = stream->loom;
		stream->clock_offset = loom->clock_offset;
	}

	fclose(f);

	err("loaded clock offsets ok\n");
}

static void
write_row_cpu(struct ovni_emu *emu)
{
	char path[PATH_MAX];

	sprintf(path, "%s/%s", emu->tracedir, "cpu.row");

	FILE *f = fopen(path, "w");

	if (f == NULL) {
		perror("cannot open row file");
		exit(EXIT_FAILURE);
	}

	fprintf(f, "LEVEL NODE SIZE 1\n");
	fprintf(f, "hostname\n");
	fprintf(f, "\n");

	fprintf(f, "LEVEL THREAD SIZE %ld\n", emu->total_ncpus);

	for (size_t i = 0; i < emu->total_ncpus; i++) {
		struct ovni_cpu *cpu = emu->global_cpu[i];
		fprintf(f, "%s\n", cpu->name);
	}

	fclose(f);
}

static void
write_row_thread(struct ovni_emu *emu)
{
	char path[PATH_MAX];

	sprintf(path, "%s/%s", emu->tracedir, "thread.row");

	FILE *f = fopen(path, "w");

	if (f == NULL) {
		perror("cannot open row file");
		exit(EXIT_FAILURE);
	}

	fprintf(f, "LEVEL NODE SIZE 1\n");
	fprintf(f, "hostname\n");
	fprintf(f, "\n");

	fprintf(f, "LEVEL THREAD SIZE %ld\n", emu->total_nthreads);

	for (size_t i = 0; i < emu->total_nthreads; i++) {
		struct ovni_ethread *th = emu->global_thread[i];
		fprintf(f, "THREAD %d.%d\n", th->proc->appid, th->tid);
	}

	fclose(f);
}

static void
init_threads(struct ovni_emu *emu)
{
	emu->total_nthreads = 0;
	emu->total_nprocs = 0;

	struct ovni_trace *trace = &emu->trace;

	/* Count total processes and threads */
	for (size_t i = 0; i < trace->nlooms; i++) {
		struct ovni_loom *loom = &trace->loom[i];
		for (size_t j = 0; j < loom->nprocs; j++) {
			struct ovni_eproc *proc = &loom->proc[j];
			emu->total_nprocs++;
			for (size_t k = 0; k < proc->nthreads; k++) {
				emu->total_nthreads++;
			}
		}
	}

	emu->global_thread = calloc(emu->total_nthreads,
			sizeof(*emu->global_thread));

	if (emu->global_thread == NULL) {
		perror("calloc failed");
		exit(EXIT_FAILURE);
	}

	int gi = 0;

	/* Populate global_thread array */
	for (size_t i = 0; i < trace->nlooms; i++) {
		struct ovni_loom *loom = &trace->loom[i];
		for (size_t j = 0; j < loom->nprocs; j++) {
			struct ovni_eproc *proc = &loom->proc[j];
			for (size_t k = 0; k < proc->nthreads; k++) {
				struct ovni_ethread *thread = &proc->thread[k];

				emu->global_thread[gi++] = thread;
			}
		}
	}
}

static void
init_cpus(struct ovni_emu *emu)
{
	struct ovni_trace *trace = &emu->trace;

	emu->global_cpu = calloc(emu->total_ncpus,
			sizeof(*emu->global_cpu));

	if (emu->global_cpu == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	for (size_t i = 0; i < trace->nlooms; i++) {
		struct ovni_loom *loom = &trace->loom[i];
		for (size_t j = 0; j < loom->ncpus; j++) {
			struct ovni_cpu *cpu = &loom->cpu[j];
			emu->global_cpu[cpu->gindex] = cpu;

			if (snprintf(cpu->name, MAX_CPU_NAME, "CPU %ld.%ld", i, j)
					>= MAX_CPU_NAME) {
				err("error cpu %ld.%ld name too long\n", i, j);
				exit(EXIT_FAILURE);
			}
			cpu->virtual = 0;
		}

		emu->global_cpu[loom->vcpu.gindex] = &loom->vcpu;
		if (snprintf(loom->vcpu.name, MAX_CPU_NAME, "CPU %ld.*", i)
				>= MAX_CPU_NAME) {
			err("error cpu %ld.* name too long\n", i);
			exit(EXIT_FAILURE);
		}
		loom->vcpu.virtual = 1;
	}
}

static void
create_pcf_cpus(struct ovni_emu *emu)
{
	/* Only needed for the thread PCF */
	struct pcf_file *pcf = &emu->pcf[CHAN_TH];
	int prvtype = chan_to_prvtype[CHAN_OVNI_CPU];
	struct pcf_type *type = pcf_find_type(pcf, prvtype);

	if (type == NULL)
		die("cannot find PCF type for CHAN_OVNI_CPU\n");

	for (size_t i = 0; i < emu->total_ncpus; i++) {
		int value = i + 1;
		char *label = emu->global_cpu[i]->name;

		pcf_add_value(type, value, label);
	}
}

static void
emu_init(struct ovni_emu *emu, int argc, char *argv[])
{
	memset(emu, 0, sizeof(*emu));

	parse_args(emu, argc, argv);

	if (ovni_load_trace(&emu->trace, emu->tracedir)) {
		err("error loading ovni trace\n");
		exit(EXIT_FAILURE);
	}

	if (ovni_load_streams(&emu->trace)) {
		err("error loading streams\n");
		exit(EXIT_FAILURE);
	}

	load_metadata(emu);

	load_clock_offsets(emu);

	init_threads(emu);
	init_cpus(emu);

	open_prvs(emu, emu->tracedir);
	open_pcfs(emu, emu->tracedir);

	create_pcf_cpus(emu);

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
	pcf_write(&emu->pcf[CHAN_TH]);
	pcf_write(&emu->pcf[CHAN_CPU]);

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

void
edie(struct ovni_emu *emu, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "fatal: ");
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "fatal: while evaluating the event %c%c%c with clock=%ld in thread=%d\n",
			emu->cur_ev->header.model,
			emu->cur_ev->header.category,
			emu->cur_ev->header.value,
			emu->cur_ev->header.clock,
			emu->cur_thread->tid);

	abort();
}

void
eerr(struct ovni_emu *emu, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	fprintf(stderr, "fatal: ");
	vfprintf(stderr, fmt, args);
	va_end(args);

	fprintf(stderr, "fatal: while evaluating the event %c%c%c with clock=%ld in thread=%d\n",
			emu->cur_ev->header.model,
			emu->cur_ev->header.category,
			emu->cur_ev->header.value,
			emu->cur_ev->header.clock,
			emu->cur_thread->tid);
}

int
main(int argc, char *argv[])
{
	struct ovni_emu *emu = malloc(sizeof(struct ovni_emu));

	if (emu == NULL) {
		perror("malloc");
		return 1;
	}

	emu_init(emu, argc, argv);

	emulate(emu);

	emu_post(emu);

	emu_destroy(emu);

	free(emu);

	err("ovniemu finished\n");

	return 0;
}
