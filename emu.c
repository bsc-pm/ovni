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
#include "chan.h"

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

	dbg(">>> %d.%d.%d %c %c %c % 20ld % 15ld ",
			stream->loom, stream->proc, stream->tid,
			ev->header.model, ev->header.category, ev->header.value, clock, delta);

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
emit_channels(struct ovni_emu *emu)
{
	struct ovni_cpu *cpu;
	struct ovni_chan *chan_cpu, *chan_th;
	struct ovni_ethread *th, *last_th;
	int i, j, k, nrunning, st;

	/* Compute the CPU derived channels from the threads */
	for(i=0; i<CHAN_MAX; i++)
	{
		for(j=0; j<emu->total_ncpus; j++)
		{
			cpu = emu->global_cpu[j];
			chan_cpu = &cpu->chan[i];

			/* Not implemented */
			assert(chan_cpu->track != CHAN_TRACK_TH_UNPAUSED);

			if(chan_cpu->track != CHAN_TRACK_TH_RUNNING)
			{
				//dbg("cpu chan %d not tracking threads\n", i);
				break;
			}

			/* Count running threads */
			for(k=0, nrunning=0; k<cpu->nthreads; k++)
			{
				th = cpu->thread[k];
				if(th->state == TH_ST_RUNNING)
				{
					last_th = th;
					nrunning++;
				}
			}

			if(nrunning == 0)
			{
				//dbg("cpu chan %d disabled: no running threads\n", i);
				if(chan_is_enabled(chan_cpu))
					chan_enable(chan_cpu, 0);
			}
			else if(nrunning > 1)
			{
				//dbg("cpu chan %d bad: multiple threads\n", i);
				if(!chan_is_enabled(chan_cpu))
					chan_enable(chan_cpu, 1);
				chan_set(chan_cpu, ST_BAD);
			}
			else
			{
				th = last_th;
				chan_th = &th->chan[i];

				//dbg("cpu chan %d good: one thread\n", i);
				if(!chan_is_enabled(chan_cpu))
					chan_enable(chan_cpu, 1);

				if(chan_is_enabled(chan_th))
					st = chan_get_st(&last_th->chan[i]);
				else
					st = ST_BAD;

				/* Only update the CPU chan if needed */
				if(chan_get_st(chan_cpu) != st)
					chan_set(chan_cpu, st);
			}
		}
	}

	/* Emit only the channels that have been updated. We need a list
	 * of updated channels */
	for(i=0; i<emu->total_nthreads; i++)
		for(j=0; j<CHAN_MAX; j++)
			chan_emit(&emu->global_thread[i]->chan[j]);

	for(i=0; i<emu->total_ncpus; i++)
		for(j=0; j<CHAN_MAX; j++)
			chan_emit(&emu->global_cpu[i]->chan[j]);

}

static void
hook_init(struct ovni_emu *emu)
{
	hook_init_ovni(emu);
	hook_init_nosv(emu);
	hook_init_tampi(emu);
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
		case 'T': hook_pre_tampi(emu);
			  break;
		default:
			break;
	}
}

static void
hook_emit(struct ovni_emu *emu)
{
	//emu_emit(emu);

	emit_channels(emu);

	switch(emu->cur_ev->header.model)
	{
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

	hook_init(emu);

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
	struct ovni_cpu *cpu;

	/* The logical id must match our index */
	assert(i == loom->ncpus);

	cpu = &loom->cpu[i];

	assert(cpu->state == CPU_ST_UNKNOWN);

	cpu->state = CPU_ST_READY;
	cpu->i = i;
	cpu->phyid = phyid;
	cpu->gindex = emu->total_ncpus++;

	dbg("new cpu %d at phyid=%d\n", cpu->gindex, phyid);

	loom->ncpus++;
}

void
proc_load_cpus(struct ovni_emu *emu, struct ovni_loom *loom, struct ovni_eproc *proc)
{
	JSON_Array *cpuarray;
	JSON_Object *cpu;
	JSON_Object *meta;
	int i, index, phyid;
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

	dbg("new vcpu %d\n", vcpu->gindex);
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

	fprintf(f, "LEVEL THREAD SIZE %d\n", emu->total_ncpus);

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
	int i, j;
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

	fprintf(f, "LEVEL THREAD SIZE %d\n", emu->total_nthreads);

	for(i=0; i<emu->total_nthreads; i++)
	{
		th = emu->global_thread[i];
		fprintf(f, "THREAD %d.%d\n", th->proc->appid, th->tid);
	}

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
	ev->header.category = mcv[1];
	ev->header.value = mcv[2];
	ev->header.clock = emu->cur_ev->header.clock;

	trace->nvirtual++;
}

static void
init_threads(struct ovni_emu *emu)
{
	struct ovni_trace *trace;
	struct ovni_loom *loom;
	struct ovni_eproc *proc;
	struct ovni_ethread *thread;
	int i, j, k, gi;

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
		perror("calloc");
		abort();
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
	int i, j, gi;

	trace = &emu->trace;

	emu->global_cpu = calloc(emu->total_ncpus,
			sizeof(*emu->global_cpu));

	if(emu->global_cpu == NULL)
	{
		perror("calloc");
		abort();
	}

	for(gi=0, i=0; i<trace->nlooms; i++)
	{
		loom = &trace->loom[i];
		for(j=0; j<loom->ncpus; j++)
		{
			cpu = &loom->cpu[j];
			emu->global_cpu[cpu->gindex] = cpu;

			if(snprintf(cpu->name, MAX_CPU_NAME, "CPU %d.%d",
						i, j) >= MAX_CPU_NAME)
			{
				abort();
			}
		}

		emu->global_cpu[loom->vcpu.gindex] = &loom->vcpu;
		if(snprintf(loom->vcpu.name, MAX_CPU_NAME, "CPU %d.*",
					i) >= MAX_CPU_NAME)
		{
			abort();
		}
	}
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

	init_threads(emu);
	init_cpus(emu);

	open_prvs(emu, emu->tracedir);
	open_pcfs(emu, emu->tracedir);

	err("loaded %d cpus and %d threads\n",
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
