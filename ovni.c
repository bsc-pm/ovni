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

#include <errno.h>
#include <fcntl.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "ovni.h"
#include "common.h"
#include "parson.h"

#ifndef gettid
# include <sys/syscall.h>
# define gettid() ((pid_t)syscall(SYS_gettid))
#endif

/* Data per process */
struct ovni_rproc rproc = {0};

/* Data per thread */
_Thread_local struct ovni_rthread rthread = {0};

static void
create_trace_dirs(char *tracedir, const char *loom, int pid)
{
	char path[PATH_MAX];

	snprintf(path, PATH_MAX, "%s", tracedir);

	/* May fail if another loom created the directory already */
	mkdir(path, 0755);

	snprintf(path, PATH_MAX, "%s/loom.%s", tracedir, loom);

	/* Also may fail */
	mkdir(path, 0755);

	snprintf(rproc.dir, PATH_MAX, "%s/loom.%s/proc.%d", tracedir, loom, pid);

	/* But this one shall not fail */
	if(mkdir(rproc.dir, 0755))
		die("mkdir %s failed: %s\n", rproc.dir, strerror(errno));
}

static void
create_trace_stream(void)
{
	char path[PATH_MAX];

	int written = snprintf(path, PATH_MAX, "%s/thread.%d",
			rproc.dir, rthread.tid);

       	if(written >= PATH_MAX)
		die("thread trace path too long: %s/thread.%d\n",
				rproc.dir, rthread.tid);

	rthread.streamfd = open(path, O_WRONLY | O_CREAT, 0644);

	if(rthread.streamfd == -1)
		die("open %s failed: %s\n", path, strerror(errno));
}

static void
proc_metadata_init(struct ovni_rproc *proc)
{
	proc->meta = json_value_init_object();

	if(proc->meta == NULL)
		die("failed to create metadata JSON object\n");
}

static void
proc_metadata_store(struct ovni_rproc *proc)
{
	char path[PATH_MAX];

	if(proc->meta == NULL)
		die("process metadata not initialized\n");
       
	if(snprintf(path, PATH_MAX, "%s/metadata.json", proc->dir) >= PATH_MAX)
		die("metadata path too long: %s/metadata.json\n", proc->dir);

	if(json_serialize_to_file_pretty(proc->meta, path) != JSONSuccess)
		die("failed to write process metadata\n");
}

void
ovni_add_cpu(int index, int phyid)
{
	if(index < 0)
		die("ovni_add_cpu: cannot use negative index %d\n", index);

	if(phyid < 0)
		die("ovni_add_cpu: cannot use negative CPU id %d\n", phyid);

	if(!rproc.ready)
		die("ovni_add_cpu: process not yet initialized\n");

	if(rproc.meta == NULL)
		die("ovni_add_cpu: metadata not initialized\n");

	JSON_Object *meta = json_value_get_object(rproc.meta);

	if(meta == NULL)
		die("ovni_add_cpu: json_value_get_object() failed\n");

	int first_time = 0;

	/* Find the CPU array and create it if needed  */
	JSON_Array *cpuarray = json_object_dotget_array(meta, "cpus");

	if(cpuarray == NULL)
	{
		JSON_Value *value = json_value_init_array();
		if(value == NULL)
			die("ovni_add_cpu: json_value_init_array() failed\n");

		cpuarray = json_array(value);
		if(cpuarray == NULL)
			die("ovni_add_cpu: json_array() failed\n");

		first_time = 1;
	}

	JSON_Value *valcpu = json_value_init_object();
	if(valcpu == NULL)
		die("ovni_add_cpu: json_value_init_object() failed\n");

	JSON_Object *cpu = json_object(valcpu);
	if(cpu == NULL)
		die("ovni_add_cpu: json_object() failed\n");

	if(json_object_set_number(cpu, "index", index) != 0)
		die("ovni_add_cpu: json_object_set_number() failed\n");

	if(json_object_set_number(cpu, "phyid", phyid) != 0)
		die("ovni_add_cpu: json_object_set_number() failed\n");

	if(json_array_append_value(cpuarray, valcpu) != 0)
		die("ovni_add_cpu: json_array_append_value() failed\n");

	if(first_time)
	{
		JSON_Value *value = json_array_get_wrapping_value(cpuarray);
		if(value == NULL)
			die("ovni_add_cpu: json_array_get_wrapping_value() failed\n");

		if(json_object_set_value(meta, "cpus", value) != 0)
			die("ovni_add_cpu: json_object_set_value failed\n");
	}
}

static void
proc_set_app(int appid)
{
	JSON_Object *meta = json_value_get_object(rproc.meta);

	if(meta == NULL)
		die("json_value_get_object failed\n");

	if(json_object_set_number(meta, "app_id", appid) != 0)
		die("json_object_set_number for app_id failed\n");
}

void
ovni_proc_init(int app, const char *loom, int pid)
{
	if(rproc.ready)
		die("ovni_proc_init: pid %d already initialized\n", pid);

	memset(&rproc, 0, sizeof(rproc));

	if(strlen(loom) >= OVNI_MAX_HOSTNAME)
		die("ovni_proc_init: loom name too long: %s\n", loom);

	strcpy(rproc.loom, loom);
	rproc.pid = pid;
	rproc.app = app;
	rproc.clockid = CLOCK_MONOTONIC;

	create_trace_dirs(OVNI_TRACEDIR, loom, pid);

	proc_metadata_init(&rproc);

	rproc.ready = 1;

	proc_set_app(app);
}

void
ovni_proc_fini(void)
{
	if(!rproc.ready)
		die("ovni_proc_fini: process not initialized\n");

	proc_metadata_store(&rproc);
}

void
ovni_thread_init(pid_t tid)
{
	if(rthread.ready)
	{
		err("warning: thread %d already initialized, ignored\n", tid);
		return;
	}

	if(tid == 0)
		die("ovni_thread_init: cannot use tid=%d\n", tid);

	if(!rproc.ready)
		die("ovni_thread_init: process not yet initialized\n");

	memset(&rthread, 0, sizeof(rthread));

	rthread.tid = tid;
	rthread.evlen = 0;
	rthread.evbuf = malloc(OVNI_MAX_EV_BUF);

	if(rthread.evbuf == NULL)
		die("ovni_thread_init: malloc failed: %s", strerror(errno));

	create_trace_stream();

	rthread.ready = 1;
}

void
ovni_thread_free(void)
{
	if(!rthread.ready)
		die("ovni_thread_free: thread not initialized\n");

	free(rthread.evbuf);
}

int
ovni_thread_isready(void)
{
	return rthread.ready;
}

#ifdef USE_TSC
static inline
uint64_t clock_tsc_now(void)
{
    uint32_t lo, hi;

    /* RDTSC copies contents of 64-bit TSC into EDX:EAX */
    __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return (uint64_t) hi << 32 | lo;
}
#endif

static uint64_t
clock_monotonic_now(void)
{
	uint64_t ns = 1000ULL * 1000ULL * 1000ULL;
	struct timespec tp;

	if(clock_gettime(rproc.clockid, &tp))
		die("clock_gettime() failed: %s\n", strerror(errno));

	return tp.tv_sec * ns + tp.tv_nsec;
}

uint64_t
ovni_clock_now(void)
{
#ifdef USE_TSC
	return clock_tsc_now();
#else
	return clock_monotonic_now();
#endif
}

static void
write_evbuf(uint8_t *buf, size_t size)
{
	do
	{
		ssize_t written = write(rthread.streamfd, buf, size);

		if(written < 0)
			die("failed to write buffer to disk: %s\n", strerror(errno));

		size -= written;
		buf += written;
	} while(size > 0);
}

static void
flush_evbuf(void)
{
	write_evbuf(rthread.evbuf, rthread.evlen);

	rthread.evlen = 0;
}

void
ovni_ev_set_clock(struct ovni_ev *ev, uint64_t clock)
{
	ev->header.clock = clock;
}

uint64_t
ovni_ev_get_clock(const struct ovni_ev *ev)
{
	return ev->header.clock;
}

void
ovni_ev_set_mcv(struct ovni_ev *ev, const char *mcv)
{
	ev->header.model = mcv[0];
	ev->header.category = mcv[1];
	ev->header.value = mcv[2];
}

static size_t
get_jumbo_payload_size(const struct ovni_ev *ev)
{
	return sizeof(ev->payload.jumbo.size) + ev->payload.jumbo.size;
}

int
ovni_payload_size(const struct ovni_ev *ev)
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
ovni_payload_add(struct ovni_ev *ev, const uint8_t *buf, int size)
{
	if(ev->header.flags & OVNI_EV_JUMBO)
		die("ovni_payload_add: event is marked as jumbo\n");

	if(size < 2)
		die("ovni_payload_add: payload size %d too small\n", size);

	size_t payload_size = ovni_payload_size(ev);

	/* Ensure we have room */
	if(payload_size + size > sizeof(ev->payload))
		die("ovni_payload_add: no space left for %d bytes\n", size);

	memcpy(&ev->payload.u8[payload_size], buf, size);
	payload_size += size;

	ev->header.flags = (ev->header.flags & 0xf0) | ((payload_size-1) & 0x0f);
}

int
ovni_ev_size(const struct ovni_ev *ev)
{
	return sizeof(ev->header) + ovni_payload_size(ev);
}

static void
ovni_ev_add(struct ovni_ev *ev);

void
ovni_flush(void)
{
	struct ovni_ev pre={0}, post={0};

	if(!rthread.ready)
		die("ovni_flush: thread is not initialized\n");

	if(!rproc.ready)
		die("ovni_flush: process is not initialized\n");

	ovni_ev_set_clock(&pre, ovni_clock_now());
	ovni_ev_set_mcv(&pre, "OF[");

	flush_evbuf();

	ovni_ev_set_clock(&post, ovni_clock_now());
	ovni_ev_set_mcv(&post, "OF]");

	/* Add the two flush events */
	ovni_ev_add(&pre);
	ovni_ev_add(&post);
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
ovni_ev_add_jumbo(struct ovni_ev *ev, const uint8_t *buf, uint32_t bufsize)
{
	int flushed = 0;
	uint64_t t0, t1;

	if(ovni_payload_size(ev) != 0)
		die("ovni_ev_add_jumbo: the event payload must be empty\n");

	ovni_payload_add(ev, (uint8_t *) &bufsize, sizeof(bufsize));
	size_t evsize = ovni_ev_size(ev);

	size_t totalsize = evsize + bufsize;

	if(totalsize >= OVNI_MAX_EV_BUF)
		die("ovni_ev_add_jumbo: event too large\n");

	/* Check if the event fits or flush first otherwise */
	if(rthread.evlen + totalsize >= OVNI_MAX_EV_BUF)
	{
		/* Measure the flush times */
		t0 = ovni_clock_now();
		flush_evbuf();
		t1 = ovni_clock_now();
		flushed = 1;
	}

	/* Set the jumbo flag here, so we capture the previous evsize
	 * properly, ignoring the jumbo buffer */
	ev->header.flags |= OVNI_EV_JUMBO;

	memcpy(&rthread.evbuf[rthread.evlen], ev, evsize);
	rthread.evlen += evsize;
	memcpy(&rthread.evbuf[rthread.evlen], buf, bufsize);
	rthread.evlen += bufsize;

	if(flushed)
	{
		/* Emit the flush events *after* the user event */
		add_flush_events(t0, t1);
	}
}

static void
ovni_ev_add(struct ovni_ev *ev)
{
	int flushed = 0;
	uint64_t t0, t1;

	int size = ovni_ev_size(ev);

	/* Check if the event fits or flush first otherwise */
	if(rthread.evlen + size >= OVNI_MAX_EV_BUF)
	{
		/* Measure the flush times */
		t0 = ovni_clock_now();
		flush_evbuf();
		t1 = ovni_clock_now();
		flushed = 1;
	}

	memcpy(&rthread.evbuf[rthread.evlen], ev, size);
	rthread.evlen += size;

	if(flushed)
	{
		/* Emit the flush events *after* the user event */
		add_flush_events(t0, t1);
	}
}

void
ovni_ev_jumbo_emit(struct ovni_ev *ev, const uint8_t *buf, uint32_t bufsize)
{
	ovni_ev_add_jumbo(ev, buf, bufsize);
}

void
ovni_ev_emit(struct ovni_ev *ev)
{
	ovni_ev_add(ev);
}
