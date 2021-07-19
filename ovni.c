#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <linux/limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdatomic.h>

#include "ovni.h"
#include "def.h"

/* Data per process */
struct ovniproc ovniproc = {0};

/* Data per thread */
_Thread_local struct ovnithr ovnithr = {0};

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

	snprintf(ovniproc.dir, PATH_MAX, "%s/loom.%d/proc.%d", tracedir, loom, proc);

	if(mkdir(ovniproc.dir, 0755))
	{
		fprintf(stderr, "mkdir %s: %s\n", ovniproc.dir, strerror(errno));
		return -1;
	}

	return 0;
}

static int
create_trace_streams(int ncpus)
{
	char path[PATH_MAX];
	int i;

	for(i=0; i<ncpus; i++)
	{
		snprintf(path, PATH_MAX, "%s/cpu.%d", ovniproc.dir, i);
		if((ovniproc.cpustream[i] = fopen(path, "w")) == NULL)
		{
			perror("fopen");
			return -1;
		}
	}

	return 0;
}

int
ovni_init(int loom, int proc, int ncpus)
{
	int i;

	fprintf(stderr, "ovni_init\n");
	memset(&ovniproc, 0, sizeof(ovniproc));
	memset(&ovnithr, 0, sizeof(ovnithr));

	for(i=0; i<MAX_CPU; i++)
		ovniproc.opened[i] = ATOMIC_VAR_INIT(0);

	ovniproc.loom = loom;
	ovniproc.proc = proc;
	ovniproc.ncpus = ncpus;

	/* By default we use the monotonic clock */
	ovniproc.clockid = CLOCK_MONOTONIC;

	if(create_trace_dirs(TRACEDIR, loom, proc))
		return -1;

	if(create_trace_streams(ncpus))
		return -1;

	return 0;
}

void
ovni_cpu_set(int cpu)
{
	ovnithr.cpu = cpu;
}

/* Sets the current time so that all subsequent events have the new
 * timestamp */
int
ovni_clock_update()
{
	struct timespec tp;
	uint64_t ns = 1000LL * 1000LL * 1000LL;
	uint64_t raw;

	if(clock_gettime(ovniproc.clockid, &tp))
		return -1;

	raw = tp.tv_sec * ns + tp.tv_nsec;
	//raw = raw >> 6;
	ovnithr.clockvalue = (uint64_t) raw;

	return 0;
}

//static void
//pack_int64(uint8_t **q, int64_t n)
//{
//	uint8_t *p = *q;
//
//	*p++ = (n >>  0) & 0xff;
//	*p++ = (n >>  8) & 0xff;
//	*p++ = (n >> 16) & 0xff;
//	*p++ = (n >> 24) & 0xff;
//	*p++ = (n >> 32) & 0xff;
//	*p++ = (n >> 40) & 0xff;
//	*p++ = (n >> 48) & 0xff;
//	*p++ = (n >> 56) & 0xff;
//
//	*q = p;
//}

static void
pack_uint32(uint8_t **q, uint32_t n)
{
	uint8_t *p = *q;

	*p++ = (n >>  0) & 0xff;
	*p++ = (n >>  8) & 0xff;
	*p++ = (n >> 16) & 0xff;
	*p++ = (n >> 24) & 0xff;

	*q = p;
}

static void
pack_uint8(uint8_t **q, uint8_t n)
{
	uint8_t *p = *q;

	*p++ = n;

	*q = p;
}

static int
ovni_write(uint8_t *buf, size_t size)
{
	FILE *f;
	int i, j;

	printf("writing %ld bytes in cpu=%d\n", size, ovnithr.cpu);

	for(i=0; i<size; i+=16)
	{
		for(j=0; j<16 && i+j < size; j++)
		{
			printf("%02x ", buf[i+j]);
		}
		printf("\n");
	}

	f = ovniproc.cpustream[ovnithr.cpu];

	if(fwrite(buf, 1, size, f) != size)
	{
		perror("fwrite");
		return -1;
	}

	fflush(f);

	return 0;
}

static int
ovni_ev(uint8_t fsm, uint8_t event, int32_t data)
{
	struct ovnievent ev;

	ev.clock = ovnithr.clockvalue;
	ev.fsm = fsm;
	ev.event = event;
	ev.data = data;

	return ovni_write((uint8_t *) &ev, sizeof(ev));
}

int
ovni_ev_worker(uint8_t fsm, uint8_t event, int32_t data)
{
	return ovni_ev(fsm, event, data);
}

void
ovni_stream_open(int cpu)
{
	if(atomic_fetch_add(&ovniproc.opened[cpu], 1) + 1 > 1)
	{
		fprintf(stderr, "the stream for cpu.%d is already in use\n", cpu);
		abort();
	}
}

void
ovni_stream_close(int cpu)
{
	atomic_fetch_sub(&ovniproc.opened[cpu], 1);
}
