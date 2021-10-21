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

#include "ovni.h"
#include "ovni_trace.h"
#include "emu.h"
#include "prv.h"
#include "pcf.h"

/* Obtains the corrected clock of the given event */
int64_t
evclock(struct ovni_stream *stream, struct ovni_ev *ev)
{
	return (int64_t) ovni_ev_get_clock(ev) + stream->clock_offset;
}

static void
emit_ev(struct ovni_stream *stream, struct ovni_ev *ev)
{
	int64_t delta;
	uint64_t clock;
	int i, payloadsize;

	//dbg("sizeof(*ev) = %d\n", sizeof(*ev));
	//hexdump((uint8_t *) ev, sizeof(*ev));

	clock = evclock(stream, ev);

	delta = clock - stream->lastclock;

	dbg("%d.%d.%d %c %c %c % 20ld % 15ld ",
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
		case 'O': hook_pre_ovni(emu);
			  break;
		case 'V': hook_pre_nosv(emu);
			  break;
		case '*': hook_pre_nosv(emu);
			  hook_pre_ovni(emu);
			  break;
		default:
			break;
	}
}

static void
hook_emit(struct ovni_emu *emu)
{
	//emu_emit(emu);

	switch(emu->cur_ev->header.model)
	{
		case 'O': hook_emit_ovni(emu);
			  break;
		case 'V': hook_emit_nosv(emu);
			  break;
		case '*': hook_emit_nosv(emu);
			  hook_emit_ovni(emu);
			  break;
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
		case 'O': hook_post_ovni(emu);
			  break;
		case 'V': hook_post_nosv(emu);
			  break;
		case '*': hook_post_nosv(emu);
			  hook_post_ovni(emu);
			  break;
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

	static int done_first = 0;

	trace = &emu->trace;

	/* Look for virtual events first */

	if(trace->ivirtual < trace->nvirtual)
	{
		emu->cur_ev = &trace->virtual_events[trace->ivirtual];
		trace->ivirtual++;
		return 0;
	}
	else
	{
		/* Reset virtual events to 0 */
		trace->ivirtual = 0;
		trace->nvirtual = 0;
	}

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
		if(f < 0 || evclock(stream, ev) < minclock)
		{
			f = i;
			minclock = evclock(stream, ev);
		}
	}

	if(f < 0)
		return -1;

	/* We have a valid stream with a new event */
	stream = &trace->stream[f];

	set_current(emu, stream);

	if(emu->lastclock > evclock(stream, stream->cur_ev))
	{
		fprintf(stdout, "warning: backwards jump in time %lu -> %lu\n",
				emu->lastclock, evclock(stream, stream->cur_ev));
	}

	emu->lastclock = evclock(stream, stream->cur_ev);

	if(!done_first)
	{
		done_first = 1;
		emu->firstclock = emu->lastclock;
	}

	emu->delta_time = emu->lastclock - emu->firstclock;

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
		emu_emit(emu);
		hook_pre(emu);
		hook_emit(emu);
		hook_post(emu);

		/* Read the next event if no virtual events exist */
		if(emu->trace.ivirtual == emu->trace.nvirtual)
			ovni_load_next_event(emu->cur_stream);
	}
}

struct ovni_ethread *
emu_get_thread(struct ovni_eproc *proc, int tid)
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
		loom->offset_ncpus = emu->total_cpus;

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
open_pcfs(struct ovni_emu *emu, char *tracedir)
{
	char path[PATH_MAX];

	sprintf(path, "%s/%s", tracedir, "thread.pcf");

	emu->pcf_thread = fopen(path, "w");

	if(emu->pcf_thread == NULL)
		abort();

	sprintf(path, "%s/%s", tracedir, "cpu.pcf");

	emu->pcf_cpu = fopen(path, "w");

	if(emu->pcf_cpu == NULL)
		abort();
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

	while((opt = getopt(argc, argv, "c:")) != -1)
	{
		switch(opt)
		{
			case 'c':
				emu->clock_offset_file = optarg;
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
	int i;
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
	int i, rank, ret;
	double offset, mean, std;
	char host[OVNI_MAX_HOSTNAME];
	struct ovni_loom *loom;
	struct ovni_trace *trace;
	struct ovni_stream *stream;
	
	f = fopen(emu->clock_offset_file, "r");

	if(f == NULL)
	{
		perror("fopen clock offset file failed\n");
		abort();
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
			abort();
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
	int i, j;
	char path[PATH_MAX];
	struct ovni_loom *loom;
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

	/* Add an extra row for the virtual CPU */
	fprintf(f, "LEVEL THREAD SIZE %d\n", emu->total_cpus + 1);

	for(i=0; i<emu->trace.nlooms; i++)
	{
		loom = &emu->trace.loom[i];
		for(j=0; j<loom->ncpus; j++)
		{
			cpu = &loom->cpu[j];
			fprintf(f, "CPU %d.%d\n", i, cpu->i);
		}
	}

	fprintf(f, "UNSET\n");

	fclose(f);
}

static int
emu_virtual_init(struct ovni_emu *emu)
{
	struct ovni_trace *trace;

	trace = &emu->trace;

	trace->ivirtual = 0;
	trace->nvirtual = 0;

	trace->virtual_events = calloc(MAX_VIRTUAL_EVENTS,
			sizeof(struct ovni_ev));

	if(trace->virtual_events == NULL)
	{
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	return 0;
}

void
emu_virtual_ev(struct ovni_emu *emu, char *mcv)
{
	struct ovni_trace *trace;
	struct ovni_ev *ev;

	trace = &emu->trace;

	if(trace->nvirtual >= MAX_VIRTUAL_EVENTS)
	{
		err("too many virtual events\n");
		exit(EXIT_FAILURE);
	}

	ev = &trace->virtual_events[trace->nvirtual];

	ev->header.flags = 0;
	ev->header.model = mcv[0];
	ev->header.class = mcv[1];
	ev->header.value = mcv[2];
	ev->header.clock = emu->cur_ev->header.clock;

	trace->nvirtual++;
}

static void
emu_init(struct ovni_emu *emu, int argc, char *argv[])
{
	memset(emu, 0, sizeof(*emu));

	parse_args(emu, argc, argv);

	if(ovni_load_trace(&emu->trace, emu->tracedir))
		abort();

	if(emu_virtual_init(emu))
		abort();

	if(ovni_load_streams(&emu->trace))
		abort();

	if(load_metadata(emu) != 0)
		abort();

	if(emu->clock_offset_file != NULL)
		load_clock_offsets(emu);

	open_prvs(emu, emu->tracedir);
	open_pcfs(emu, emu->tracedir);
}

static void
emu_post(struct ovni_emu *emu)
{
	/* Write the PCF files */
	pcf_write(emu->pcf_thread);
	pcf_write(emu->pcf_cpu);

	write_row_cpu(emu);
}

static void
emu_destroy(struct ovni_emu *emu)
{
	close_prvs(emu);
	close_pcfs(emu);
	destroy_metadata(emu);
	ovni_free_streams(&emu->trace);
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
