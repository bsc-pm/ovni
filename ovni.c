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
#include "common.h"
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

#ifdef USE_RDTSC
static inline
uint64_t rdtsc(void)
{
    uint32_t lo, hi;

    // RDTSC copies contents of 64-bit TSC into EDX:EAX
    __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return (uint64_t) hi << 32 | lo;
}
#endif

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
