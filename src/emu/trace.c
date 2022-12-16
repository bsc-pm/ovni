/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "trace.h"

#define _GNU_SOURCE
#include <unistd.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static int
find_dir_prefix_str(const char *dirname, const char *prefix, const char **str)
{
	const char *p = dirname;

	/* Check the prefix */
	if (strncmp(p, prefix, strlen(prefix)) != 0)
		return -1;

	p += strlen(prefix);

	/* Find the dot */
	if (*p != '.')
		return -1;

	p++;

	if (str)
		*str = p;

	return 0;
}

static int
find_dir_prefix_int(const char *dirname, const char *prefix, int *num)
{
	const char *p = NULL;

	if (find_dir_prefix_str(dirname, prefix, &p) != 0)
		return -1;

	/* Convert the suffix string to a number */
	*num = atoi(p);

	return 0;
}

static size_t
count_dir_prefix(DIR *dir, const char *prefix)
{
	struct dirent *dirent;
	size_t n = 0;

	while ((dirent = readdir(dir)) != NULL) {
		if (find_dir_prefix_str(dirent->d_name, prefix, NULL) != 0)
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

	if (strlen(filepath) >= PATH_MAX) {
		err("filepath too large: %s\n", filepath);
		return -1;
	}

	strcpy(thread->tracefile, filepath);

	return 0;
}

static void
load_proc_metadata(struct ovni_eproc *proc, int *rank_enabled)
{
	JSON_Object *meta = json_value_get_object(proc->meta);
	if (meta == NULL)
		die("load_proc_metadata: json_value_get_object() failed\n");

	JSON_Value *appid_val = json_object_get_value(meta, "app_id");
	if (appid_val == NULL)
		die("process %d is missing app_id in metadata\n", proc->pid);

	proc->appid = (int) json_number(appid_val);

	JSON_Value *rank_val = json_object_get_value(meta, "rank");

	if (rank_val != NULL) {
		proc->rank = (int) json_number(rank_val);
		*rank_enabled = 1;
	} else {
		proc->rank = -1;
	}
}

static void
check_metadata_version(struct ovni_eproc *proc)
{
	JSON_Object *meta = json_value_get_object(proc->meta);
	if (meta == NULL)
		die("check_metadata_version: json_value_get_object() failed\n");

	JSON_Value *version_val = json_object_get_value(meta, "version");
	if (version_val == NULL) {
		die("process %d is missing attribute \"version\" in metadata\n",
				proc->pid);
	}

	int version = (int) json_number(version_val);

	if (version != OVNI_METADATA_VERSION) {
		die("pid %d: metadata version mismatch %d (expected %d)\n",
				proc->pid, version,
				OVNI_METADATA_VERSION);
	}

	JSON_Value *mversion_val = json_object_get_value(meta, "model_version");
	if (mversion_val == NULL) {
		die("process %d is missing attribute \"model_version\" in metadata\n",
				proc->pid);
	}

	const char *mversion = json_string(mversion_val);

	if (strcmp(mversion, OVNI_MODEL_VERSION) != 0) {
		die("pid %d: metadata model version mismatch '%s' (expected '%s')\n",
				proc->pid, mversion,
				OVNI_MODEL_VERSION);
	}
}

static int
compare_int(const void *a, const void *b)
{
	int aa = *(const int *) a;
	int bb = *(const int *) b;

	if (aa < bb)
		return -1;
	else if (aa > bb)
		return +1;
	else
		return 0;
}

static int
load_proc(struct ovni_eproc *proc, struct ovni_loom *loom, int index, int pid, char *procdir)
{
	static int total_procs = 0;

	proc->pid = pid;
	proc->index = index;
	proc->gindex = total_procs++;
	proc->loom = loom;

	char path[PATH_MAX];
	if (snprintf(path, PATH_MAX, "%s/%s", procdir, "metadata.json") >= PATH_MAX) {
		err("snprintf: path too large: %s\n", procdir);
		abort();
	}

	proc->meta = json_parse_file_with_comments(path);
	if (proc->meta == NULL) {
		err("error loading metadata from %s\n", path);
		return -1;
	}

	check_metadata_version(proc);

	/* The appid is populated from the metadata */
	load_proc_metadata(proc, &loom->rank_enabled);

	DIR *dir;
	if ((dir = opendir(procdir)) == NULL) {
		fprintf(stderr, "opendir %s failed: %s\n",
				procdir, strerror(errno));
		return -1;
	}

	proc->nthreads = count_dir_prefix(dir, "thread");

	if (proc->nthreads <= 0) {
		err("cannot find any thread for process %d\n",
				proc->pid);
		return -1;
	}

	proc->thread = calloc(proc->nthreads, sizeof(struct ovni_ethread));

	if (proc->thread == NULL) {
		perror("calloc failed");
		return -1;
	}

	int *tids;

	if ((tids = calloc(proc->nthreads, sizeof(int))) == NULL) {
		perror("calloc failed\n");
		return -1;
	}

	rewinddir(dir);

	for (size_t i = 0; i < proc->nthreads;) {
		struct dirent *dirent = readdir(dir);

		if (dirent == NULL) {
			err("inconsistent: readdir returned NULL\n");
			return -1;
		}

		if (find_dir_prefix_int(dirent->d_name, "thread", &tids[i]) != 0)
			continue;

		i++;
	}

	closedir(dir);

	/* Sort threads by ascending TID */
	qsort(tids, proc->nthreads, sizeof(int), compare_int);

	for (size_t i = 0; i < proc->nthreads; i++) {
		int tid = tids[i];

		if (snprintf(path, PATH_MAX, "%s/thread.%d", procdir, tid) >= PATH_MAX) {
			err("snprintf: path too large: %s\n", procdir);
			abort();
		}

		struct ovni_ethread *thread = &proc->thread[i];

		if (load_thread(thread, proc, i, tid, path) != 0)
			return -1;
	}

	free(tids);

	return 0;
}

static int
load_loom(struct ovni_loom *loom, char *loomdir)
{
	DIR *dir = NULL;

	if ((dir = opendir(loomdir)) == NULL) {
		fprintf(stderr, "opendir %s failed: %s\n",
				loomdir, strerror(errno));
		return -1;
	}

	loom->rank_enabled = 0;
	loom->nprocs = count_dir_prefix(dir, "proc");

	if (loom->nprocs <= 0) {
		err("cannot find any process directory in loom %s\n",
				loom->hostname);
		return -1;
	}

	loom->proc = calloc(loom->nprocs, sizeof(struct ovni_eproc));

	if (loom->proc == NULL) {
		perror("calloc failed");
		return -1;
	}

	rewinddir(dir);

	size_t i = 0;
	struct dirent *dirent = NULL;
	while ((dirent = readdir(dir)) != NULL) {
		int pid;
		if (find_dir_prefix_int(dirent->d_name, "proc", &pid) != 0)
			continue;

		if (i >= loom->nprocs) {
			err("more process than expected\n");
			abort();
		}

		struct ovni_eproc *proc = &loom->proc[i];

		sprintf(proc->dir, "%s/%s", loomdir, dirent->d_name);

		if (load_proc(&loom->proc[i], loom, i, pid, proc->dir) != 0)
			return -1;

		i++;
	}

	if (i != loom->nprocs) {
		err("unexpected number of processes\n");
		abort();
	}

	closedir(dir);

	/* Ensure all process have the rank, if enabled in any */
	if (loom->rank_enabled) {
		for (i = 0; i < loom->nprocs; i++) {
			struct ovni_eproc *proc = &loom->proc[i];
			if (proc->rank < 0) {
				die("process %d is missing the rank\n",
						proc->pid);
			}
		}
	}

	return 0;
}

static int
compare_looms(const void *a, const void *b)
{
	struct ovni_loom *la = (struct ovni_loom *) a;
	struct ovni_loom *lb = (struct ovni_loom *) b;
	return strcmp(la->dname, lb->dname);
}

static void
loom_to_host(const char *loom_name, char *host, int n)
{
	int i = 0;
	for (i = 0; i < n; i++) {
		/* Copy until dot or end */
		if (loom_name[i] != '.' && loom_name[i] != '\0')
			host[i] = loom_name[i];
		else
			break;
	}

	if (i == n)
		die("loom host name %s too long\n", loom_name);

	host[i] = '\0';
}

int
ovni_load_trace(struct ovni_trace *trace, char *tracedir)
{
	DIR *dir = NULL;

	if ((dir = opendir(tracedir)) == NULL) {
		err("opendir %s failed: %s\n", tracedir, strerror(errno));
		return -1;
	}

	trace->nlooms = count_dir_prefix(dir, "loom");

	if (trace->nlooms == 0) {
		err("cannot find any loom in %s\n", tracedir);
		return -1;
	}

	trace->loom = calloc(trace->nlooms, sizeof(struct ovni_loom));

	if (trace->loom == NULL) {
		perror("calloc failed\n");
		return -1;
	}

	rewinddir(dir);

	size_t l = 0;
	struct dirent *dirent = NULL;

	while ((dirent = readdir(dir)) != NULL) {
		struct ovni_loom *loom = &trace->loom[l];
		const char *loom_name;
		if (find_dir_prefix_str(dirent->d_name, "loom", &loom_name) != 0) {
			/* Ignore other files in tracedir */
			continue;
		}

		if (l >= trace->nlooms) {
			err("extra loom detected\n");
			return -1;
		}

		/* Copy the complete loom directory name to looms */
		if (snprintf(loom->dname, PATH_MAX, "%s", dirent->d_name) >= PATH_MAX) {
			err("error: loom name %s too long\n", dirent->d_name);
			return -1;
		}

		l++;
	}

	closedir(dir);

	/* Sort the looms, so we get the hostnames in alphanumeric order */
	qsort(trace->loom, trace->nlooms, sizeof(struct ovni_loom),
			compare_looms);

	for (size_t i = 0; i < trace->nlooms; i++) {
		struct ovni_loom *loom = &trace->loom[i];
		const char *name = NULL;

		if (find_dir_prefix_str(loom->dname, "loom", &name) != 0) {
			err("error: mismatch for loom %s\n", loom->dname);
			return -1;
		}

		loom_to_host(name, loom->hostname, sizeof(loom->hostname));

		if (snprintf(loom->path, PATH_MAX, "%s/%s",
				    tracedir, loom->dname)
				>= PATH_MAX) {
			err("error: loom path %s/%s too long\n",
					tracedir, loom->dname);
			return -1;
		}

		if (load_loom(loom, loom->path) != 0)
			return -1;
	}

	return 0;
}

static int
check_stream_header(struct ovni_stream *stream)
{
	int ret = 0;

	if (stream->size < sizeof(struct ovni_stream_header)) {
		err("stream %d: incomplete stream header\n",
				stream->tid);
		return -1;
	}

	struct ovni_stream_header *h =
			(struct ovni_stream_header *) stream->buf;

	if (memcmp(h->magic, OVNI_STREAM_MAGIC, 4) != 0) {
		char magic[5];
		memcpy(magic, h->magic, 4);
		magic[4] = '\0';
		err("stream %d: wrong stream magic '%s' (expected '%s')\n",
				stream->tid, magic, OVNI_STREAM_MAGIC);
		ret = -1;
	}

	if (h->version != OVNI_STREAM_VERSION) {
		err("stream %d: stream version mismatch %u (expected %u)\n",
				stream->tid, h->version, OVNI_STREAM_VERSION);
		ret = -1;
	}

	return ret;
}

static int
load_stream_fd(struct ovni_stream *stream, int fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0) {
		perror("fstat failed");
		return -1;
	}

	/* Error because it doesn't have the header */
	if (st.st_size == 0) {
		err("stream %d is empty\n", stream->tid);
		return -1;
	}

	int prot = PROT_READ | PROT_WRITE;
	stream->buf = mmap(NULL, st.st_size, prot, MAP_PRIVATE, fd, 0);

	if (stream->buf == MAP_FAILED) {
		perror("mmap failed");
		return -1;
	}

	stream->size = st.st_size;

	return 0;
}

static int
load_stream_buf(struct ovni_stream *stream, struct ovni_ethread *thread)
{
	int fd;

	if ((fd = open(thread->tracefile, O_RDWR)) == -1) {
		perror("open failed");
		return -1;
	}

	if (load_stream_fd(stream, fd) != 0)
		return -1;

	if (check_stream_header(stream) != 0) {
		err("stream %d: bad header\n", stream->tid);
		return -1;
	}

	stream->offset = sizeof(struct ovni_stream_header);

	if (stream->offset == stream->size)
		stream->active = 0;
	else
		stream->active = 1;

	/* No need to keep the fd open */
	if (close(fd)) {
		perror("close failed");
		return -1;
	}

	return 0;
}

/* Populates the streams in a single array */
int
ovni_load_streams(struct ovni_trace *trace)
{
	trace->nstreams = 0;

	for (size_t i = 0; i < trace->nlooms; i++) {
		struct ovni_loom *loom = &trace->loom[i];
		for (size_t j = 0; j < loom->nprocs; j++) {
			struct ovni_eproc *proc = &loom->proc[j];
			for (size_t k = 0; k < proc->nthreads; k++) {
				trace->nstreams++;
			}
		}
	}

	trace->stream = calloc(trace->nstreams, sizeof(struct ovni_stream));

	if (trace->stream == NULL) {
		perror("calloc failed");
		return -1;
	}

	err("loaded %ld streams\n", trace->nstreams);

	size_t s = 0;
	for (size_t i = 0; i < trace->nlooms; i++) {
		struct ovni_loom *loom = &trace->loom[i];
		for (size_t j = 0; j < loom->nprocs; j++) {
			struct ovni_eproc *proc = &loom->proc[j];
			for (size_t k = 0; k < proc->nthreads; k++) {
				struct ovni_ethread *thread = &proc->thread[k];
				struct ovni_stream *stream = &trace->stream[s++];

				stream->tid = thread->tid;
				stream->thread = thread;
				stream->proc = proc;
				stream->loom = loom;
				stream->lastclock = 0;
				stream->offset = 0;
				stream->cur_ev = NULL;

				if (load_stream_buf(stream, thread) != 0) {
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
	for (size_t i = 0; i < trace->nstreams; i++) {
		struct ovni_stream *stream = &trace->stream[i];
		if (munmap(stream->buf, stream->size) != 0)
			die("munmap stream failed: %s\n", strerror(errno));
	}

	free(trace->stream);
}

void
ovni_free_trace(struct ovni_trace *trace)
{
	for (size_t i = 0; i < trace->nlooms; i++) {
		for (size_t j = 0; j < trace->loom[i].nprocs; j++) {
			free(trace->loom[i].proc[j].thread);
		}

		free(trace->loom[i].proc);
	}

	free(trace->loom);
}

int
ovni_load_next_event(struct ovni_stream *stream)
{
	if (stream->active == 0) {
		dbg("stream is inactive, cannot load more events\n");
		return -1;
	}

	/* Only step the offset if we have load an event */
	if (stream->cur_ev != NULL)
		stream->offset += ovni_ev_size(stream->cur_ev);

	/* It cannot overflow, otherwise we are reading garbage */
	if (stream->offset > stream->size)
		die("ovni_load_next_event: stream offset exceeds size\n");

	/* We have reached the end */
	if (stream->offset == stream->size) {
		stream->active = 0;
		stream->cur_ev = NULL;
		dbg("stream %d runs out of events\n", stream->tid);
		return -1;
	}

	stream->cur_ev = (struct ovni_ev *) &stream->buf[stream->offset];

	return 0;
}
