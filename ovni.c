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
#include <dirent.h>

#include "ovni.h"

#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
#define dbg(...) fprintf(stderr, __VA_ARGS__);
#else
#define dbg(...)
#endif

#define err(...) fprintf(stderr, __VA_ARGS__);

//#define ENABLE_SLOW_CHECKS

//#define USE_RDTSC

/* Data per process */
struct ovni_rproc rproc = {0};

/* Data per thread */
_Thread_local struct ovni_rthread rthread = {0};

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

	if(create_trace_dirs(OVNI_TRACEDIR, loom, proc))
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
	rthread.evlen = 0;

	rthread.evbuf = malloc(OVNI_MAX_EV_BUF);
	if(rthread.evbuf == NULL)
	{
		perror("malloc");
		abort();
	}

	if(create_trace_stream(tid))
		abort();

	rthread.ready = 1;

	return 0;
}

int
ovni_thread_free()
{
	assert(rthread.ready);
	free(rthread.evbuf);
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

static inline
uint64_t rdtsc(void)
{
    uint32_t lo, hi;

    // RDTSC copies contents of 64-bit TSC into EDX:EAX
    asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return (uint64_t) hi << 32 | lo;
}

uint64_t
ovni_get_clock()
{
	struct timespec tp;
	uint64_t ns = 1000LL * 1000LL * 1000LL;
	uint64_t raw;
	int ret;

#ifdef USE_RDTSC
	raw = rdtsc();
#else

	ret = clock_gettime(rproc.clockid, &tp);

#ifdef ENABLE_SLOW_CHECKS
	if(ret) abort();
#endif /* ENABLE_SLOW_CHECKS */

	raw = tp.tv_sec * ns + tp.tv_nsec;
	rthread.clockvalue = (uint64_t) raw;

#endif /* USE_RDTSC */

	return raw;
}

/* Sets the current time so that all subsequent events have the new
 * timestamp */
void
ovni_clock_update()
{
	rthread.clockvalue = ovni_get_clock();
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

static int
flush_evbuf()
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
	ev->clock_lo = (uint32_t) (rthread.clockvalue & 0xffffffff);
	ev->clock_hi = (uint16_t) ((rthread.clockvalue >> 32) & 0xffff);
}

uint64_t
ovni_ev_get_clock(struct ovni_ev *ev)
{
	uint64_t clock;

	clock = ((uint64_t) ev->clock_hi) << 32 | ((uint64_t) ev->clock_lo);
	return clock;
}

void
ovni_ev_set_mcv(struct ovni_ev *ev, char *mcv)
{
	ev->model = mcv[0];
	ev->class = mcv[1];
	ev->value = mcv[2];
}

int
ovni_payload_size(struct ovni_ev *ev)
{
	return ev->flags & 0x0f;
}

void
ovni_payload_add(struct ovni_ev *ev, uint8_t *buf, int size)
{
	/* Ensure we have room */
	assert(ovni_payload_size(ev) + size < 16);

	ev->flags = ev->flags & 0xf0 | size & 0x0f;
}

int
ovni_ev_size(struct ovni_ev *ev)
{
	return sizeof(*ev) - sizeof(ev->payload) +
		ovni_payload_size(ev);
}


static void
ovni_ev_add(struct ovni_ev *ev)
{
	int size;	

	ovni_ev_set_clock(ev);
	
	size = ovni_ev_size(ev);


	memcpy(&rthread.evbuf[rthread.evlen], ev, size);
	rthread.evlen += size;
}

void
ovni_ev(struct ovni_ev *ev)
{
	ovni_ev_set_clock(ev);
	ovni_ev_add(ev);
}

int
ovni_flush()
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


static int
load_proc(struct ovni_eproc *proc, char *procdir)
{
	struct dirent *dirent;
	DIR *dir;
	char path[PATH_MAX];
	char *p;
	struct ovni_ethread *thread;

	if((dir = opendir(procdir)) == NULL)
	{
		fprintf(stderr, "opendir %s failed: %s\n",
				procdir, strerror(errno));
		return -1;
	}

	while((dirent = readdir(dir)) != NULL)
	{
		if(dirent->d_name[0] != 't')
			continue;
		p = strchr(dirent->d_name, '.');
		if(p == NULL)
			continue;
		p++;
		if(*p == '\0')
		{
			fprintf(stderr, "bad thread stream file: %s\n",
					dirent->d_name);
			return -1;
		}


		thread = &proc->thread[proc->nthreads++];
		thread->tid = atoi(p);

		sprintf(path, "%s/%s", procdir, dirent->d_name);
		thread->f = fopen(path, "r");

		if(thread->f == NULL)
		{
			fprintf(stderr, "fopen %s failed: %s\n",
					path, strerror(errno));
			return -1;
		}
	}

	closedir(dir);

	return 0;
}

static int
load_loom(struct ovni_loom *loom, char *loomdir)
{
	int proc;
	char path[PATH_MAX];
	struct stat st;

	for(proc=0; proc<OVNI_MAX_PROC; proc++)
	{
		sprintf(path, "%s/proc.%d", loomdir, proc);

		if(stat(path, &st) != 0)
		{
			/* No more proc.N directories */
			if(errno == ENOENT)
				break;

			fprintf(stderr, "cannot stat %s: %s\n", path,
					strerror(errno));
			return -1;
		}

		if(!S_ISDIR(st.st_mode))
		{
			fprintf(stderr, "not a dir %s\n", path);
			return -1;
		}

		if(load_proc(&loom->proc[proc], path))
			return -1;
	}

	loom->nprocs = proc;

	return 0;
}

int
ovni_load_trace(struct ovni_trace *trace, char *tracedir)
{
	int loom, nlooms;
	char path[PATH_MAX];

	/* TODO: For now only one loom */
	nlooms = 1;
	loom = 0;

	sprintf(path, "%s/loom.%d", tracedir, loom);

	if(load_loom(&trace->loom[loom], path))
		return -1;

	trace->nlooms = nlooms;

	return 0;
}

/* Populates the streams in a single array */
int
ovni_load_streams(struct ovni_trace *trace)
{
	int i, j, k, s;
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

	fprintf(stderr, "loaded %d streams\n", trace->nstreams);

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

				stream->f = thread->f;
				stream->tid = thread->tid;
				stream->thread = k;
				stream->proc = j;
				stream->loom = i;
				stream->active = 1;
				stream->lastclock = 0;
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

void
ovni_load_next_event(struct ovni_stream *stream)
{
	int i;
	size_t n, size;
	struct ovni_ev *ev;
	uint8_t flags;

	if(!stream->active)
		return;

	ev = &stream->last;
	if((n = fread(&ev->flags, sizeof(ev->flags), 1, stream->f)) != 1)
	{
		dbg("stream is empty\n");
		stream->active = 0;
		return;
	}

	//dbg("flags = %d\n", ev->flags);

	size = ovni_ev_size(ev) - sizeof(ev->flags);
	//dbg("ev size = %d\n", size);


	/* Clean payload from previous event */
	memset(&ev->payload, 0, sizeof(ev->payload));

	if((n = fread(((uint8_t *) ev) + sizeof(ev->flags), 1, size, stream->f)) != size)
	{
		err("warning: garbage found at the end of the stream\n");
		stream->active = 0;
		return;
	}

	//dbg("loaded next event:\n");
	//hexdump((uint8_t *) ev, ovni_ev_size(ev));
	//dbg("---------\n");

	stream->active = 1;

}
