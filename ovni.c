/*
 * MIT License
 * 
 * Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _GNU_SOURCE
#include <unistd.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <linux/limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdatomic.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "ovni.h"
#include "ovni_trace.h"
#include "parson.h"

#ifndef gettid
# include <sys/syscall.h>
# define gettid() ((pid_t)syscall(SYS_gettid))
#endif

//#define ENABLE_SLOW_CHECKS

//#define USE_RDTSC

/* Data per process */
struct ovni_rproc rproc = {0};

/* Data per thread */
_Thread_local struct ovni_rthread rthread = {0};

static int
create_trace_dirs(char *tracedir, char *loom, int proc)
{
	char path[PATH_MAX];

	snprintf(path, PATH_MAX, "%s", tracedir);

	/* May fail if another loom created the directory already */
	mkdir(path, 0755);

	snprintf(path, PATH_MAX, "%s/loom.%s", tracedir, loom);

	/* Also may fail */
	mkdir(path, 0755);

	snprintf(rproc.dir, PATH_MAX, "%s/loom.%s/proc.%d", tracedir, loom, proc);

	/* But this one shall not fail */
	if(mkdir(rproc.dir, 0755))
	{
		fprintf(stderr, "mkdir %s: %s\n", rproc.dir, strerror(errno));
		return -1;
	}

	return 0;
}

static int
create_trace_stream(void)
{
	char path[PATH_MAX];

	if(snprintf(path, PATH_MAX, "%s/thread.%d", rproc.dir, rthread.tid)
			>= PATH_MAX)
	{
		abort();
	}

	//rthread.streamfd = open(path, O_WRONLY | O_CREAT | O_DSYNC, 0644);
	rthread.streamfd = open(path, O_WRONLY | O_CREAT, 0644);

	if(rthread.streamfd == -1)
	{
		fprintf(stderr, "open %s failed: %s\n", path, strerror(errno));
		return -1;
	}

	return 0;
}

static int
proc_metadata_init(struct ovni_rproc *proc)
{
	proc->meta = json_value_init_object();

	if(proc->meta == NULL)
	{
		err("failed to create json object\n");
		abort();
	}

	return 0;
}

static int
proc_metadata_store(struct ovni_rproc *proc)
{
	char path[PATH_MAX];
       
	if(snprintf(path, PATH_MAX, "%s/metadata.json", proc->dir) >= PATH_MAX)
		abort();

	assert(proc->meta != NULL);

	if(json_serialize_to_file_pretty(proc->meta, path) != JSONSuccess)
	{
		err("failed to write proc metadata\n");
		abort();
	}

	return 0;
}

void
ovni_add_cpu(int index, int phyid)
{
	JSON_Array *cpuarray;
	JSON_Object *cpu;
	JSON_Object *meta;
	JSON_Value *valcpu, *valcpuarray;
	int append = 0;

	assert(rproc.ready == 1);
	assert(rproc.meta != NULL);

	meta = json_value_get_object(rproc.meta);

	assert(meta);

	/* Find the CPU array and create it if needed  */
	cpuarray = json_object_dotget_array(meta, "cpus");

	if(cpuarray == NULL)
	{
		valcpuarray = json_value_init_array();
		assert(valcpuarray);

		cpuarray = json_array(valcpuarray);
		assert(cpuarray);
		append = 1;
	}

	valcpuarray = json_array_get_wrapping_value(cpuarray);
	assert(valcpuarray);

	valcpu = json_value_init_object();
	assert(valcpu);

	cpu = json_object(valcpu);
	assert(cpu);

	if(json_object_set_number(cpu, "index", index) != 0)
		abort();

	if(json_object_set_number(cpu, "phyid", phyid) != 0)
		abort();

	if(json_array_append_value(cpuarray, valcpu) != 0)
		abort();

	if(append && json_object_set_value(meta, "cpus", valcpuarray) != 0)
		abort();

	//puts(json_serialize_to_string_pretty(rproc.meta));
}

static void
proc_set_app(int appid)
{
	JSON_Object *meta;

	assert(rproc.ready == 1);
	assert(rproc.meta != NULL);

	meta = json_value_get_object(rproc.meta);

	assert(meta);

	if(json_object_set_number(meta, "app_id", appid) != 0)
		abort();
}

int
ovni_proc_init(int app, char *loom, int proc)
{
	assert(rproc.ready == 0);

	memset(&rproc, 0, sizeof(rproc));

	/* FIXME: strcpy is insecure */
	strcpy(rproc.loom, loom);
	rproc.proc = proc;
	rproc.app = app;

	/* By default we use the monotonic clock */
	rproc.clockid = CLOCK_MONOTONIC;

	if(create_trace_dirs(OVNI_TRACEDIR, loom, proc))
		abort();

	if(proc_metadata_init(&rproc) != 0)
		abort();

	rproc.ready = 1;

	proc_set_app(app);

	return 0;
}

int
ovni_proc_fini(void)
{
	if(proc_metadata_store(&rproc) != 0)
		abort();

	return 0;
}

int
ovni_thread_init(pid_t tid)
{
	assert(tid != 0);

	if(rthread.ready)
	{
		fprintf(stderr, "warning: thread tid=%d already initialized\n",
				tid);
		return 0;
	}

	assert(rthread.ready == 0);
	assert(rthread.tid == 0);
	assert(rthread.cpu == 0);
	assert(rproc.ready == 1);

	memset(&rthread, 0, sizeof(rthread));

	rthread.tid = tid;
	rthread.cpu = -666;
	rthread.evlen = 0;

	rthread.evbuf = malloc(OVNI_MAX_EV_BUF);
	if(rthread.evbuf == NULL)
	{
		perror("malloc");
		abort();
	}

	if(create_trace_stream())
		abort();

	rthread.ready = 1;

	return 0;
}

void
ovni_thread_free(void)
{
	assert(rthread.ready);
	free(rthread.evbuf);
}

int
ovni_thread_isready(void)
{
	return rthread.ready;
}

static inline
uint64_t rdtsc(void)
{
    uint32_t lo, hi;

    // RDTSC copies contents of 64-bit TSC into EDX:EAX
    __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return (uint64_t) hi << 32 | lo;
}

uint64_t
ovni_get_clock(void)
{
	struct timespec tp;
	uint64_t ns = 1000LL * 1000LL * 1000LL;
	uint64_t raw;

#ifdef USE_RDTSC
	raw = rdtsc();
#else

#ifdef ENABLE_SLOW_CHECKS
	int ret = clock_gettime(rproc.clockid, &tp);
	if(ret) abort();
#else
	clock_gettime(rproc.clockid, &tp);
#endif /* ENABLE_SLOW_CHECKS */

	raw = tp.tv_sec * ns + tp.tv_nsec;
	rthread.clockvalue = (uint64_t) raw;

#endif /* USE_RDTSC */

	return raw;
}

/* Sets the current time so that all subsequent events have the new
 * timestamp */
void
ovni_clock_update(void)
{
	rthread.clockvalue = ovni_get_clock();
}

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
//			err("%02x ", buf[i+j]);
//		}
//		err("\n");
//	}
//}

static int
ovni_write(uint8_t *buf, size_t size)
{
	ssize_t written;

	do
	{
		written = write(rthread.streamfd, buf, size);

		if(written < 0)
		{
			perror("write");
			return -1;
		}

		size -= written;
		buf += written;
	} while(size > 0);

	return 0;
}

static int
flush_evbuf(void)
{
	int ret;

	assert(rthread.ready);
	assert(rproc.ready);

	ret = ovni_write(rthread.evbuf, rthread.evlen);

	rthread.evlen = 0;

	return ret;
}

static void
ovni_ev_set_clock(struct ovni_ev *ev)
{
	ev->header.clock = rthread.clockvalue;
}

uint64_t
ovni_ev_get_clock(struct ovni_ev *ev)
{
	return ev->header.clock;
}

void
ovni_ev_set_mcv(struct ovni_ev *ev, char *mcv)
{
	ev->header.model = mcv[0];
	ev->header.category = mcv[1];
	ev->header.value = mcv[2];
}

static size_t
get_jumbo_payload_size(struct ovni_ev *ev)
{
	return sizeof(ev->payload.jumbo.size) + ev->payload.jumbo.size;
}

int
ovni_payload_size(struct ovni_ev *ev)
{
	int size;

	if(ev->header.flags & OVNI_EV_JUMBO)
		return get_jumbo_payload_size(ev);

	size = ev->header.flags & 0x0f;

	if(size == 0)
		return 0;

	/* The minimum size is 2 bytes, so we can encode a length of 16
	 * bytes using 4 bits (0x0f) */
	size++;

	return size;
}

void
ovni_payload_add(struct ovni_ev *ev, uint8_t *buf, int size)
{
	size_t payload_size;

	assert((ev->header.flags & OVNI_EV_JUMBO) == 0);
	assert(size >= 2);

	payload_size = ovni_payload_size(ev);

	/* Ensure we have room */
	assert(payload_size + size <= sizeof(ev->payload));

	memcpy(&ev->payload.u8[payload_size], buf, size);
	payload_size += size;

	ev->header.flags = (ev->header.flags & 0xf0) | ((payload_size-1) & 0x0f);
}

int
ovni_ev_size(struct ovni_ev *ev)
{
	return sizeof(ev->header) + ovni_payload_size(ev);
}

static void
ovni_ev_add(struct ovni_ev *ev);

int
ovni_flush(void)
{
	int ret = 0;
	struct ovni_ev pre={0}, post={0};

	assert(rthread.ready);
	assert(rproc.ready);

	ovni_clock_update();
	ovni_ev_set_clock(&pre);
	ovni_ev_set_mcv(&pre, "OF[");

	ret = flush_evbuf();

	ovni_clock_update();
	ovni_ev_set_clock(&post);
	ovni_ev_set_mcv(&post, "OF]");

	/* Add the two flush events */
	ovni_ev_add(&pre);
	ovni_ev_add(&post);

	return ret;
}

static void
add_flush_events(uint64_t t0, uint64_t t1)
{
	struct ovni_ev pre={0}, post={0};

	pre.header.clock = t0;
	ovni_ev_set_mcv(&pre, "OF[");

	post.header.clock = t1;
	ovni_ev_set_mcv(&post, "OF]");

	/* Add the two flush events */
	ovni_ev_add(&pre);
	ovni_ev_add(&post);
}

static void
ovni_ev_add_jumbo(struct ovni_ev *ev, uint8_t *buf, uint32_t bufsize)
{
	size_t evsize, totalsize;
	int flushed = 0;
	uint64_t t0, t1;

	assert(ovni_payload_size(ev) == 0);

	ovni_payload_add(ev, (uint8_t *) &bufsize, sizeof(bufsize));
	evsize = ovni_ev_size(ev);

	totalsize = evsize + bufsize;

	/* Check if the event fits or flush first otherwise */
	if(rthread.evlen + totalsize >= OVNI_MAX_EV_BUF)
	{
		/* Measure the flush times */
		t0 = ovni_get_clock();
		flush_evbuf();
		t1 = ovni_get_clock();
		flushed = 1;
	}

	/* Se the jumbo flag here, so we capture the previous evsize
	 * properly, ignoring the jumbo buffer */
	ev->header.flags |= OVNI_EV_JUMBO;

	memcpy(&rthread.evbuf[rthread.evlen], ev, evsize);
	rthread.evlen += evsize;
	memcpy(&rthread.evbuf[rthread.evlen], buf, bufsize);
	rthread.evlen += bufsize;

	assert(rthread.evlen < OVNI_MAX_EV_BUF);

	if(flushed)
	{
		/* Emit the flush events *after* the user event */
		add_flush_events(t0, t1);

		/* Set the current clock to the last event */
		rthread.clockvalue = t1;
	}
}

static void
ovni_ev_add(struct ovni_ev *ev)
{
	int size, flushed = 0;
	uint64_t t0, t1;

	size = ovni_ev_size(ev);

	/* Check if the event fits or flush first otherwise */
	if(rthread.evlen + size >= OVNI_MAX_EV_BUF)
	{
		/* Measure the flush times */
		t0 = ovni_get_clock();
		flush_evbuf();
		t1 = ovni_get_clock();
		flushed = 1;
	}

	memcpy(&rthread.evbuf[rthread.evlen], ev, size);
	rthread.evlen += size;

	if(flushed)
	{
		/* Emit the flush events *after* the user event */
		add_flush_events(t0, t1);

		/* Set the current clock to the last event */
		rthread.clockvalue = t1;
	}
}

void
ovni_ev_jumbo_emit(struct ovni_ev *ev, uint8_t *buf, uint32_t bufsize)
{
	ovni_ev_set_clock(ev);
	ovni_ev_add_jumbo(ev, buf, bufsize);
}

void
ovni_ev_emit(struct ovni_ev *ev)
{
	ovni_ev_set_clock(ev);
	ovni_ev_add(ev);
}

static int
find_dir_prefix_str(struct dirent *dirent, const char *prefix, const char **str)
{
	const char *p;

	p = dirent->d_name;

	/* Check the prefix */
	if(strncmp(p, prefix, strlen(prefix)) != 0)
		return -1;

	p += strlen(prefix);

	/* Find the dot */
	if(*p != '.')
		return -1;

	p++;

	*str = p;

	return 0;
}

static int
find_dir_prefix_int(struct dirent *dirent, const char *prefix, int *num)
{
	const char *p;

	if(find_dir_prefix_str(dirent, prefix, &p) != 0)
		return -1;

	/* Convert the suffix string to a number */
	*num = atoi(p);

	return 0;
}

static int
load_thread(struct ovni_ethread *thread, struct ovni_eproc *proc, int index, int tid, char *filepath)
{
	static int total_threads = 0;

	thread->tid = tid;
	thread->index = index;
	thread->gindex = total_threads++;
	thread->state = TH_ST_UNKNOWN;
	thread->proc = proc;
	thread->stream_fd = open(filepath, O_RDONLY);

	if(thread->stream_fd == -1)
	{
		perror("open");
		return -1;
	}
	return 0;
}

static void
load_proc_metadata(struct ovni_eproc *proc)
{
	JSON_Object *meta;

	meta = json_value_get_object(proc->meta);
	assert(meta);

	proc->appid = (int) json_object_get_number(meta, "app_id");
}


static int
load_proc(struct ovni_eproc *proc, struct ovni_loom *loom, int index, int pid, char *procdir)
{
	static int total_procs = 0;

	struct dirent *dirent;
	DIR *dir;
	char path[PATH_MAX];
	struct ovni_ethread *thread;
	int tid;

	proc->pid = pid;
	proc->index = index;
	proc->gindex = total_procs++;
	proc->loom = loom;

	if(snprintf(path, PATH_MAX, "%s/%s", procdir, "metadata.json") >=
			PATH_MAX)
	{
		err("snprintf: path too large: %s\n", procdir);
		abort();
	}

	proc->meta = json_parse_file_with_comments(path);
	if(proc->meta == NULL)
	{
		err("error loading metadata from %s\n", path);
		return -1;
	}

	/* The appid is populated from the metadata */
	load_proc_metadata(proc);

	if((dir = opendir(procdir)) == NULL)
	{
		fprintf(stderr, "opendir %s failed: %s\n",
				procdir, strerror(errno));
		return -1;
	}

	while((dirent = readdir(dir)) != NULL)
	{
		if(find_dir_prefix_int(dirent, "thread", &tid) != 0)
			continue;

		if(snprintf(path, PATH_MAX, "%s/%s", procdir, dirent->d_name) >=
				PATH_MAX)
		{
			err("snprintf: path too large: %s\n", procdir);
			abort();
		}

		if(proc->nthreads >= OVNI_MAX_THR)
		{
			err("too many thread streams for process %d\n", pid);
			abort();
		}

		thread = &proc->thread[proc->nthreads];

		if(load_thread(thread, proc, proc->nthreads, tid, path) != 0)
			return -1;

		proc->nthreads++;
	}

	closedir(dir);



	return 0;
}

static int
load_loom(struct ovni_loom *loom, int loomid, char *loomdir)
{
	int pid;
	char path[PATH_MAX];
	DIR *dir;
	struct dirent *dirent;
	struct ovni_eproc *proc;

	if((dir = opendir(loomdir)) == NULL)
	{
		fprintf(stderr, "opendir %s failed: %s\n",
				loomdir, strerror(errno));
		return -1;
	}

	while((dirent = readdir(dir)) != NULL)
	{
		if(find_dir_prefix_int(dirent, "proc", &pid) != 0)
			continue;

		sprintf(path, "%s/%s", loomdir, dirent->d_name);

		if(loom->nprocs >= OVNI_MAX_PROC)
		{
			err("too many process streams for loom %d\n",
					loomid);
			abort();
		}

		proc = &loom->proc[loom->nprocs];

		if(load_proc(proc, loom, loom->nprocs, pid, path) != 0)
			return -1;

		loom->nprocs++;
	}

	closedir(dir);

	return 0;
}

static int
compare_alph(const void* a, const void* b)
{
	return strcmp(*(const char**)a, *(const char**)b);
}

int
ovni_load_trace(struct ovni_trace *trace, char *tracedir)
{
	char *looms[OVNI_MAX_LOOM];
	char *hosts[OVNI_MAX_LOOM];
	const char *loom_name;
	DIR *dir;
	struct dirent *dirent;
	size_t l;

	trace->nlooms = 0;

	if((dir = opendir(tracedir)) == NULL)
	{
		fprintf(stderr, "opendir %s failed: %s\n",
				tracedir, strerror(errno));
		return -1;
	}

	for(l=0; l<OVNI_MAX_LOOM; l++)
	{
		looms[l] = malloc(PATH_MAX);
		hosts[l] = malloc(PATH_MAX);
	}

	while((dirent = readdir(dir)) != NULL)
	{
		if(find_dir_prefix_str(dirent, "loom", &loom_name) != 0)
		{
			/* Ignore other files in tracedir */
			continue;
		}

		if(trace->nlooms >= OVNI_MAX_LOOM)
		{
			err("too many looms for trace %s\n",
					tracedir);
			abort();
		}

		sprintf(hosts[trace->nlooms], "%s", loom_name);

		sprintf(looms[trace->nlooms], "%s/%s", tracedir, dirent->d_name);

		trace->nlooms++;
	}

	qsort((const char **) hosts, trace->nlooms, sizeof(const char*), compare_alph);
	qsort((const char **) looms, trace->nlooms, sizeof(const char*), compare_alph);

	for(l=0; l<trace->nlooms; l++)
	{
		if(strlen(hosts[l]) >= PATH_MAX)
		{
			err("error hostname too long: %s\n", hosts[l]);
			exit(EXIT_FAILURE);
		}

		/* Safe */
		strcpy(trace->loom[l].hostname, hosts[l]);

		if(load_loom(&trace->loom[l], l, looms[l]) != 0)
			return -1;
	}

	closedir(dir);

	for(l=0; l<OVNI_MAX_LOOM; l++)
	{
		free(looms[l]);
		free(hosts[l]);
	}

	return 0;
}

static int
load_stream_buf(struct ovni_stream *stream, struct ovni_ethread *thread)
{
	struct stat st;

	if(fstat(thread->stream_fd, &st) < 0)
	{
		perror("fstat");
		return -1;
	}

	if(st.st_size == 0)
	{
		err("warning: stream %d is empty\n", stream->tid);
		stream->size = 0;
		stream->buf = NULL;
		stream->active = 0;

		return 0;
	}

	stream->size = st.st_size;
	stream->buf = mmap(NULL, stream->size, PROT_READ, MAP_SHARED,
			thread->stream_fd, 0);

	if(stream->buf == MAP_FAILED)
	{
		perror("mmap");
		return -1;
	}

	stream->active = 1;

	return 0;
}

/* Populates the streams in a single array */
int
ovni_load_streams(struct ovni_trace *trace)
{
	size_t i, j, k, s;
	struct ovni_loom *loom;
	struct ovni_eproc *proc;
	struct ovni_ethread *thread;
	struct ovni_stream *stream;

	trace->nstreams = 0;

	for(i=0; i<trace->nlooms; i++)
	{
		loom = &trace->loom[i];
		for(j=0; j<loom->nprocs; j++)
		{
			proc = &loom->proc[j];
			for(k=0; k<proc->nthreads; k++)
			{
				trace->nstreams++;
			}
		}
	}

	trace->stream = calloc(trace->nstreams, sizeof(struct ovni_stream));

	if(trace->stream == NULL)
	{
		perror("calloc");
		return -1;
	}

	err("loaded %ld streams\n", trace->nstreams);

	for(s=0, i=0; i<trace->nlooms; i++)
	{
		loom = &trace->loom[i];
		for(j=0; j<loom->nprocs; j++)
		{
			proc = &loom->proc[j];
			for(k=0; k<proc->nthreads; k++)
			{
				thread = &proc->thread[k];
				stream = &trace->stream[s++];

				stream->tid = thread->tid;
				stream->thread = k;
				stream->proc = j;
				stream->loom = i;
				stream->lastclock = 0;
				stream->offset = 0;
				stream->cur_ev = NULL;

				if(load_stream_buf(stream, thread) != 0)
				{
					err("load_stream_buf failed\n");
					return -1;
				}

			}
		}
	}

	return 0;
}

void
ovni_free_streams(struct ovni_trace *trace)
{
	free(trace->stream);
}

int
ovni_load_next_event(struct ovni_stream *stream)
{
	size_t size;

	if(stream->active == 0)
	{
		dbg("stream is inactive, cannot load more events\n");
		return -1;
	}

	if(stream->cur_ev == NULL)
	{
		stream->cur_ev = (struct ovni_ev *) stream->buf;
		stream->offset = 0;
		size = 0;
		goto out;
	}

	//printf("advancing offset %ld bytes\n", ovni_ev_size(stream->cur_ev));
	size = ovni_ev_size(stream->cur_ev);
	stream->offset += size;

	/* We have reached the end */
	if(stream->offset == stream->size)
	{
		stream->active = 0;
		dbg("stream %d runs out of events\n", stream->tid);
		return -1;
	}

	/* It cannot overflow, otherwise we are reading garbage */
	assert(stream->offset < stream->size);

	stream->cur_ev = (struct ovni_ev *) &stream->buf[stream->offset];

out:

	//dbg("---------\n");
	//dbg("ev size = %d\n", ovni_ev_size(stream->cur_ev));
	//dbg("ev flags = %02x\n", stream->cur_ev->header.flags);
	//dbg("loaded next event:\n");
	//hexdump((uint8_t *) stream->cur_ev, ovni_ev_size(stream->cur_ev));
	//dbg("---------\n");

	return (int) size;
}
