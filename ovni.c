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
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "ovni.h"
#include "def.h"

//#define ENABLE_SLOW_CHECKS

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
	int fd;
	char path[PATH_MAX];

	fprintf(stderr, "create thread stream tid=%d gettid=%d rproc.proc=%d rproc.ready=%d\n",
			rthread.tid, gettid(), rproc.proc, rproc.ready);

	snprintf(path, PATH_MAX, "%s/thread.%d", rproc.dir, rthread.tid);

	//rthread.streamfd = open(path, O_WRONLY | O_CREAT | O_DSYNC, 0644);
	rthread.streamfd = open(path, O_WRONLY | O_CREAT, 0644);

	if(rthread.streamfd == -1)
	{
		fprintf(stderr, "open %s failed: %s\n", path, strerror(errno));
		/* Shall we just return -1 ? */
		abort();
		//return -1;
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
	rthread.nevents = 0;

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
void
ovni_clock_update()
{
	struct timespec tp;
	uint64_t ns = 1000LL * 1000LL * 1000LL;
	uint64_t raw;
	int ret;

	ret = clock_gettime(rproc.clockid, &tp);

#ifdef ENABLE_SLOW_CHECKS
	if(ret) abort();
#endif

	raw = tp.tv_sec * ns + tp.tv_nsec;
	//raw = raw >> 6;
	rthread.clockvalue = (uint64_t) raw;
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

int
ovni_thread_flush()
{
	int ret = 0;
	struct event pre={0}, post={0};

	assert(rthread.ready);
	assert(rproc.ready);

	ovni_clock_update();
	pre.clock = rthread.clockvalue;
	pre.fsm  = 'F';
	pre.event = '[';

	ret = ovni_write((uint8_t *) rthread.events, rthread.nevents * sizeof(struct event));
	rthread.nevents = 0;

	ovni_clock_update();
	post.clock = rthread.clockvalue;
	post.fsm = 'F';
	post.event = ']';

	/* Also emit the two flush events */
	memcpy(&rthread.events[rthread.nevents++], &pre, sizeof(struct event));;
	memcpy(&rthread.events[rthread.nevents++], &post, sizeof(struct event));;

	return ret;
}

static int
ovni_ev(uint8_t fsm, uint8_t event, uint16_t a, uint16_t b)
{
	struct event *ev;
	int ret = 0;

	ev = &rthread.events[rthread.nevents++];

	ev->clock = rthread.clockvalue;
	ev->fsm = fsm;
	ev->event = event;
	ev->a = a;
	ev->b = b;

	/* Flush */
	if(rthread.nevents >= MAX_EV)
		ret = ovni_thread_flush();

	return ret;
}

int
ovni_thread_ev(uint8_t fsm, uint8_t event, uint16_t a, uint16_t b)
{
	assert(rthread.ready);
	assert(rproc.ready);
	return ovni_ev(fsm, event, a, b);
}

