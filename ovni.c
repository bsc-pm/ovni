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
#include <stdatomic.h>
#include <assert.h>

#include "ovni.h"
#include "def.h"

/* Data per process */
struct rproc rproc = {0};

/* Data per thread */
_Thread_local struct rthread rthread = {0};

static int
create_trace_dirs(char *tracedir, int loom, int proc)
{
	char path[PATH_MAX];

	fprintf(stderr, "create trace dirs for loom=%d, proc=%d\n",
			loom, proc);

	snprintf(path, PATH_MAX, "%s", tracedir);

	if(mkdir(path, 0755))
	{
		fprintf(stderr, "mkdir %s: %s\n", path, strerror(errno));
		//return -1;
	}

	snprintf(path, PATH_MAX, "%s/loom.%d", tracedir, loom);

	if(mkdir(path, 0755))
	{
		fprintf(stderr, "mkdir %s: %s\n", path, strerror(errno));
		//return -1;
	}

	snprintf(rproc.dir, PATH_MAX, "%s/loom.%d/proc.%d", tracedir, loom, proc);

	if(mkdir(rproc.dir, 0755))
	{
		fprintf(stderr, "mkdir %s: %s\n", rproc.dir, strerror(errno));
		return -1;
	}

	return 0;
}

static int
create_trace_stream()
{
	char path[PATH_MAX];

	fprintf(stderr, "create thread stream tid=%d gettid=%d rproc.proc=%d rproc.ready=%d\n",
			rthread.tid, gettid(), rproc.proc, rproc.ready);

	snprintf(path, PATH_MAX, "%s/thread.%d", rproc.dir, rthread.tid);
	if((rthread.stream = fopen(path, "w")) == NULL)
	{
		fprintf(stderr, "fopen %s failed: %s\n", path, strerror(errno));
		abort();
		return -1;
	}

	return 0;
}

int
ovni_proc_init(int loom, int proc)
{
	int i;

	assert(rproc.ready == 0);

	memset(&rproc, 0, sizeof(rproc));

	rproc.loom = loom;
	rproc.proc = proc;

	/* By default we use the monotonic clock */
	rproc.clockid = CLOCK_MONOTONIC;

	if(create_trace_dirs(TRACEDIR, loom, proc))
		abort();

	rproc.ready = 1;

	return 0;
}

int
ovni_thread_init(pid_t tid)
{
	int i;

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

	fprintf(stderr, "ovni thread init tid=%d\n", tid);

	memset(&rthread, 0, sizeof(rthread));

	rthread.tid = tid;
	rthread.cpu = -666;

	if(create_trace_stream(tid))
		abort();

	rthread.ready = 1;

	return 0;
}

int
ovni_thread_isready()
{
	return rthread.ready;
}

void
ovni_cpu_set(int cpu)
{
	rthread.cpu = cpu;
}

/* Sets the current time so that all subsequent events have the new
 * timestamp */
int
ovni_clock_update()
{
	struct timespec tp;
	uint64_t ns = 1000LL * 1000LL * 1000LL;
	uint64_t raw;

	if(clock_gettime(rproc.clockid, &tp))
		return -1;

	raw = tp.tv_sec * ns + tp.tv_nsec;
	//raw = raw >> 6;
	rthread.clockvalue = (uint64_t) raw;

	return 0;
}

static void
hexdump(uint8_t *buf, size_t size)
{
	int i, j;

	//printf("writing %ld bytes in cpu=%d\n", size, rthread.cpu);

	for(i=0; i<size; i+=16)
	{
		for(j=0; j<16 && i+j < size; j++)
		{
			printf("%02x ", buf[i+j]);
		}
		printf("\n");
	}
}

static int
ovni_write(uint8_t *buf, size_t size)
{
	assert(rthread.stream);

	fprintf(stderr, "writing %ld bytes in thread.%d\n", size, rthread.tid);
	if(fwrite(buf, 1, size, rthread.stream) != size)
	{
		perror("fwrite");
		return -1;
	}

	//fflush(rthread.stream);

	return 0;
}

static int
ovni_ev(uint8_t fsm, uint8_t event, uint16_t a, uint16_t b)
{
	struct event ev;

	ev.clock = rthread.clockvalue;
	ev.fsm = fsm;
	ev.event = event;
	ev.a = a;
	ev.b = b;

	return ovni_write((uint8_t *) &ev, sizeof(ev));
}

int
ovni_thread_ev(uint8_t fsm, uint8_t event, uint16_t a, uint16_t b)
{
	assert(rthread.ready);
	assert(rproc.ready);
	return ovni_ev(fsm, event, a, b);
}
