#include "trace.h"

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
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

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

	if(str)
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
count_dir_prefix(DIR *dir, const char *prefix)
{
	struct dirent *dirent;
	int n = 0;

	while((dirent = readdir(dir)) != NULL)
	{
		if(find_dir_prefix_str(dirent, prefix, NULL) != 0)
			continue;

		n++;
	}

	return n;
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

	if(strlen(filepath) >= PATH_MAX)
	{
		err("filepath too large: %s\n", filepath);
		return -1;
	}

	strcpy(thread->tracefile, filepath);

	return 0;
}

static void
load_proc_metadata(struct ovni_eproc *proc, int *rank_enabled)
{
	JSON_Object *meta;

	meta = json_value_get_object(proc->meta);
	if(meta == NULL)
		die("load_proc_metadata: json_value_get_object() failed\n");

	JSON_Value *appid_val = json_object_get_value(meta, "app_id");
	if(appid_val == NULL)
		die("process %d is missing app_id in metadata\n", proc->pid);

	proc->appid = (int) json_number(appid_val);

	JSON_Value *rank_val = json_object_get_value(meta, "rank");

	if(rank_val != NULL)
	{
		proc->rank = (int) json_number(rank_val);
		*rank_enabled = 1;
	}
	else
	{
		proc->rank = -1;
	}
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
	size_t i;

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
	load_proc_metadata(proc, &loom->rank_enabled);

	if((dir = opendir(procdir)) == NULL)
	{
		fprintf(stderr, "opendir %s failed: %s\n",
				procdir, strerror(errno));
		return -1;
	}

	proc->nthreads = count_dir_prefix(dir, "thread");

	if(proc->nthreads <= 0)
	{
		err("cannot find any thread for process %d\n",
				proc->pid);
		return -1;
	}

	proc->thread = calloc(proc->nthreads, sizeof(struct ovni_ethread));

	if(proc->thread == NULL)
	{
		perror("calloc failed");
		return -1;
	}

	rewinddir(dir);

	i = 0;
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

		thread = &proc->thread[i];

		if(load_thread(thread, proc, i, tid, path) != 0)
			return -1;

		i++;
	}

	closedir(dir);

	return 0;
}

static int
load_loom(struct ovni_loom *loom, char *loomdir)
{
	int pid;
	size_t i;
	char path[PATH_MAX];
	DIR *dir;
	struct dirent *dirent;

	if((dir = opendir(loomdir)) == NULL)
	{
		fprintf(stderr, "opendir %s failed: %s\n",
				loomdir, strerror(errno));
		return -1;
	}

	loom->rank_enabled = 0;
	loom->nprocs = count_dir_prefix(dir, "proc");

	if(loom->nprocs <= 0)
	{
		err("cannot find any process directory in loom %s\n",
				loom->hostname);
		return -1;
	}

	loom->proc = calloc(loom->nprocs, sizeof(struct ovni_eproc));

	if(loom->proc == NULL)
	{
		perror("calloc failed");
		return -1;
	}

	rewinddir(dir);

	i = 0;
	while((dirent = readdir(dir)) != NULL)
	{
		if(find_dir_prefix_int(dirent, "proc", &pid) != 0)
			continue;

		sprintf(path, "%s/%s", loomdir, dirent->d_name);

		if(i >= loom->nprocs)
		{
			err("more process than expected\n");
			abort();
		}

		if(load_proc(&loom->proc[i], loom, i, pid, path) != 0)
			return -1;

		i++;

	}

	if(i != loom->nprocs)
	{
		err("unexpected number of processes\n");
		abort();
	}

	closedir(dir);

	/* Ensure all process have the rank, if enabled in any */
	if(loom->rank_enabled)
	{
		for(i = 0; i < loom->nprocs; i++)
		{
			struct ovni_eproc *proc = &loom->proc[i];
			if(proc->rank < 0)
			{
				die("process %d is missing the rank\n",
						proc->pid);
			}
		}
	}

	return 0;
}

static int
compare_alph(const void *a, const void *b)
{
	return strcmp((const char *)a, (const char *)b);
}

int
ovni_load_trace(struct ovni_trace *trace, char *tracedir)
{
	char (*looms)[PATH_MAX];
	char (*hosts)[PATH_MAX];
	const char *loom_name;
	DIR *dir;
	struct dirent *dirent;
	size_t l;

	trace->nlooms = 0;

	if((dir = opendir(tracedir)) == NULL)
	{
		err("opendir %s failed: %s\n", tracedir, strerror(errno));
		return -1;
	}

	/* Find how many looms we have */
	while((dirent = readdir(dir)) != NULL)
	{
		if(find_dir_prefix_str(dirent, "loom", &loom_name) != 0)
		{
			/* Ignore other files in tracedir */
			continue;
		}

		trace->nlooms++;
	}

	if(trace->nlooms == 0)
	{
		err("cannot find any loom in %s\n", tracedir);
		return -1;
	}

	/* Then allocate the loom array */
	trace->loom = calloc(trace->nlooms, sizeof(struct ovni_loom));

	if(trace->loom == NULL)
	{
		perror("calloc failed\n");
		return -1;
	}

	if((looms = calloc(trace->nlooms, PATH_MAX)) == NULL)
	{
		perror("calloc failed\n");
		return -1;
	}

	if((hosts = calloc(trace->nlooms, PATH_MAX)) == NULL)
	{
		perror("calloc failed\n");
		return -1;
	}

	rewinddir(dir);

	l = 0;

	while((dirent = readdir(dir)) != NULL)
	{
		if(find_dir_prefix_str(dirent, "loom", &loom_name) != 0)
		{
			/* Ignore other files in tracedir */
			continue;
		}

		if(l >= trace->nlooms)
		{
			err("extra loom detected\n");
			return -1;
		}

		if(snprintf(hosts[l], PATH_MAX, "%s",
					loom_name) >= PATH_MAX)
		{
			err("error: hostname %s too long\n", loom_name);
			return -1;
		}

		if(snprintf(looms[l], PATH_MAX, "%s/%s",
					tracedir, dirent->d_name) >= PATH_MAX)
		{
			err("error: loom name %s too long\n", loom_name);
			return -1;
		}

		l++;
	}

	closedir(dir);

	qsort(hosts, trace->nlooms, PATH_MAX, compare_alph);
	qsort(looms, trace->nlooms, PATH_MAX, compare_alph);

	for(l=0; l<trace->nlooms; l++)
	{
		if(strlen(hosts[l]) >= PATH_MAX)
		{
			err("error hostname too long: %s\n", hosts[l]);
			return -1;
		}

		/* Safe */
		strcpy(trace->loom[l].hostname, hosts[l]);

		if(load_loom(&trace->loom[l], looms[l]) != 0)
			return -1;
	}

	free(looms);
	free(hosts);

	return 0;
}

static int
load_stream_buf(struct ovni_stream *stream, struct ovni_ethread *thread)
{
	struct stat st;
	int fd;

	if((fd = open(thread->tracefile, O_RDWR)) == -1)
	{
		perror("open failed");
		return -1;
	}

	if(fstat(fd, &st) < 0)
	{
		perror("fstat failed");
		return -1;
	}

	if(st.st_size == 0)
	{
		err("warning: stream %d is empty\n", stream->tid);
		stream->size = 0;
		stream->buf = NULL;
		stream->active = 0;

		/* No need to do anything else */
		return 0;
	}

	stream->size = st.st_size;
	stream->buf = mmap(NULL, stream->size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0);

	if(stream->buf == MAP_FAILED)
	{
		perror("mmap failed");
		return -1;
	}

	stream->active = 1;

	/* No need to keep the fd open */
	if(close(fd))
	{
		perror("close failed");
		return -1;
	}

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
		perror("calloc failed");
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

void
ovni_free_trace(struct ovni_trace *trace)
{
	size_t i, j;

	for(i=0; i<trace->nlooms; i++)
	{
		for(j=0; j<trace->loom[i].nprocs; j++)
		{
			free(trace->loom[i].proc[j].thread);
		}

		free(trace->loom[i].proc);
	}

	free(trace->loom);
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

	/* It cannot overflow, otherwise we are reading garbage */
	if(stream->offset > stream->size)
		die("ovni_load_next_event: stream offset exceeds size\n");

	/* We have reached the end */
	if(stream->offset == stream->size)
	{
		stream->active = 0;
		dbg("stream %d runs out of events\n", stream->tid);
		return -1;
	}

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
