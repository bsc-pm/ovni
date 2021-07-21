#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdatomic.h>
#include <dirent.h> 

#include "def.h"

int load_proc(struct eproc *proc, char *procdir)
{
	struct dirent *dirent;
	DIR *dir;
	char path[PATH_MAX];
	char *p;
	struct ethread *thread;

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

int load_loom(struct loom *loom, char *loomdir)
{
	int proc;
	char path[PATH_MAX];
	struct stat st;

	for(proc=0; proc<MAX_PROC; proc++)
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

int load_trace(struct trace *trace, char *tracedir)
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
int load_streams(struct trace *trace)
{
	int i, j, k, s;
	struct loom *loom;
	struct eproc *proc;
	struct ethread *thread;
	struct stream *stream;

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

	trace->stream = calloc(trace->nstreams, sizeof(struct stream));

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
			}
		}
	}

	return 0;
}

int load_first_event(struct trace *trace)
{
	int i;
	struct stream *stream;
	struct event *ev;

	for(i=0; i<trace->nstreams; i++)
	{
		stream = &trace->stream[i];
		ev = &stream->last;
		if(fread(ev, sizeof(*ev), 1, stream->f) != 1)
		{
			fprintf(stderr, "failed to read and event\n");
			return -1;
		}

		fprintf(stderr, "ev clock %u\n", ev->clock);
	}

	return 0;
}

void emit(struct stream *stream, struct event *ev)
{
	static uint64_t firstclock = 0;
	int64_t delta;
	int task;

	if(firstclock == 0)
		firstclock = ev->clock;

	delta = ev->clock - firstclock;

	//#Paraver (19/01/38 at 03:14):00000000000000000000_ns:0:1:1(00000000000000000008:1)
	//2:0:1:1:7:1540663:6400010:1
	//2:0:1:1:7:1540663:6400015:1
	//2:0:1:1:7:1540663:6400017:0
	//2:0:1:1:7:1542091:6400010:1
	//2:0:1:1:7:1542091:6400015:1
	//2:0:1:1:7:1542091:6400025:1
	//2:0:1:1:7:1542091:6400017:0

	printf("2:0:1:1:%d:%ld:%d:%d\n", stream->thread+1, delta, ev->fsm, ev->event);
}

void load_next_event(struct stream *stream)
{
	int i;
	size_t n;
	struct event *ev;

	if(!stream->active)
		return;

	ev = &stream->last;
	if((n = fread(ev, sizeof(*ev), 1, stream->f)) != 1)
	{
		//fprintf(stderr, "failed to read an event, n=%ld\n", n);
		stream->active = 0;
		return;
	}

	stream->active = 1;
}

void dump_events(struct trace *trace)
{
	int i, f;
	uint64_t minclock, lastclock;
	struct event *ev;
	struct stream *stream;

	/* Load events */
	for(i=0; i<trace->nstreams; i++)
	{
		stream = &trace->stream[i];
		load_next_event(stream);
	}

	lastclock = 0;

	while(1)
	{
		f = -1;
		minclock = 0;

		/* Select next event based on the clock */
		for(i=0; i<trace->nstreams; i++)
		{
			stream = &trace->stream[i];

			if(!stream->active)
				continue;

			ev = &stream->last;
			if(f < 0 || ev->clock < minclock)
			{
				f = i;
				minclock = ev->clock;
			}
		}

		//fprintf(stderr, "f=%d minclock=%u\n", f, minclock);

		if(f < 0)
			break;

		stream = &trace->stream[f];

		if(lastclock >= stream->last.clock)
		{
			fprintf(stderr, "warning: backwards jump in time\n");
		}

		/* Emit current event */
		emit(stream, &stream->last);

		lastclock = stream->last.clock;

		/* Read the next one */
		load_next_event(stream);

		/* Unset the index */
		f = -1;
		minclock = 0;

	}
}

void free_streams(struct trace *trace)
{
	free(trace->stream);
}

int main(int argc, char *argv[])
{
	char *tracedir;
	struct trace trace;

	if(argc != 2)
	{
		fprintf(stderr, "missing tracedir\n");
		exit(EXIT_FAILURE);
	}

	tracedir = argv[1];

	if(load_trace(&trace, tracedir))
		return 1;

	if(load_streams(&trace))
		return 1;

	printf("#Paraver (19/01/38 at 03:14):00000000000000000000_ns:0:1:1(%d:1)\n", trace.nstreams);

	dump_events(&trace);

	free_streams(&trace);

	return 0;
}
