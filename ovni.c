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

	snprintf(path, PATH_MAX, "%s/thread.%d", rproc.dir, rthread.tid);
	if((rthread.stream = fopen(path, "w")) == NULL)
	{
		fprintf(stderr, "fopen %s failed: %s\n", path, strerror(errno));
		return -1;
	}

	return 0;
}

int
ovni_proc_init(int loom, int proc, int ncpus)
{
	int i;
	memset(&rproc, 0, sizeof(rproc));

	rproc.loom = loom;
	rproc.proc = proc;
	rproc.ncpus = ncpus;

	/* By default we use the monotonic clock */
	rproc.clockid = CLOCK_MONOTONIC;

	if(create_trace_dirs(TRACEDIR, loom, proc))
		return -1;

	return 0;
}

int
ovni_thread_init(pid_t tid)
{
	int i;

	assert(tid != 0);

	memset(&rthread, 0, sizeof(rthread));

	rthread.tid = tid;
	rthread.cpu = -1;
	rthread.state = ST_THREAD_INIT;

	if(create_trace_stream(tid))
		return -1;

	return 0;
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
	fprintf(stderr, "writing %ld bytes in thread.%d\n", size, rthread.tid);
	if(fwrite(buf, 1, size, rthread.stream) != size)
	{
		perror("fwrite");
		return -1;
	}

	fflush(rthread.stream);

	return 0;
}

static int
ovni_ev(uint8_t fsm, uint8_t event, int32_t data)
{
	struct event ev;

	ev.clock = rthread.clockvalue;
	ev.fsm = fsm;
	ev.event = event;
	ev.data = data;

	return ovni_write((uint8_t *) &ev, sizeof(ev));
}

int
ovni_ev_worker(uint8_t fsm, uint8_t event, int32_t data)
{
	assert(rthread.state == ST_THREAD_INIT);
	return ovni_ev(fsm, event, data);
}
