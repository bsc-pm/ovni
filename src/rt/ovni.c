/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: MIT */

#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "ovni.h"
#include "parson.h"
#include "version.h"

/* Data per process */
struct ovni_rproc rproc = {0};

/* Data per thread */
_Thread_local struct ovni_rthread rthread = {0};

void ovni_version_check_str(const char *version)
{
	if (version == NULL)
		die("ovni version string is NULL");

	int provided[3];
	int expected[3];

	if (version_parse(version, provided) != 0)
		die("failed to parse provided version \"%s\"", version);

	if (version_parse(OVNI_LIB_VERSION, expected) != 0)
		die("failed to parse expected version \"%s\"", OVNI_LIB_VERSION);

	/* Match the major */
	if (provided[0] != expected[0]) {
		die("ovni major version mismatch: found %d (%s), expected %d (%s)",
				provided[0], version,
				expected[0], OVNI_LIB_VERSION);
	}

	/* Only fail if the minor is older */
	if (provided[1] < expected[1]) {
		die("ovni minor version too old: found %d (%s), expected %d (%s)",
				provided[1], version,
				expected[1], OVNI_LIB_VERSION);
	}

	/* Ignore the patch number */
}

static void
create_trace_stream(void)
{
	char path[PATH_MAX];

	int written = snprintf(path, PATH_MAX, "%s/thread.%d%s",
			rproc.procdir, rthread.tid, OVNI_STREAM_EXT);

	if (written >= PATH_MAX)
		die("thread trace path too long: %s/thread.%d%s",
				rproc.procdir, rthread.tid, OVNI_STREAM_EXT);

	rthread.streamfd = open(path, O_WRONLY | O_CREAT, 0644);

	if (rthread.streamfd == -1)
		die("open %s failed:", path);
}

static void
proc_metadata_init(struct ovni_rproc *proc)
{
	proc->meta = json_value_init_object();

	if (proc->meta == NULL)
		die("failed to create metadata JSON object");
}

static void
proc_metadata_store(JSON_Value *meta, const char *procdir)
{
	char path[PATH_MAX];

	if (meta == NULL)
		die("process metadata not initialized");

	if (snprintf(path, PATH_MAX, "%s/metadata.json", procdir) >= PATH_MAX)
		die("metadata path too long: %s/metadata.json",
				procdir);

	if (json_serialize_to_file_pretty(meta, path) != JSONSuccess)
		die("failed to write process metadata");
}

void
ovni_add_cpu(int index, int phyid)
{
	if (index < 0)
		die("cannot use negative index %d", index);

	if (phyid < 0)
		die("cannot use negative CPU id %d", phyid);

	if (!rproc.ready)
		die("process not yet initialized");

	if (rproc.meta == NULL)
		die("metadata not initialized");

	JSON_Object *meta = json_value_get_object(rproc.meta);

	if (meta == NULL)
		die("json_value_get_object() failed");

	int first_time = 0;

	/* Find the CPU array and create it if needed */
	JSON_Array *cpuarray = json_object_dotget_array(meta, "cpus");

	if (cpuarray == NULL) {
		JSON_Value *value = json_value_init_array();
		if (value == NULL)
			die("json_value_init_array() failed");

		cpuarray = json_array(value);
		if (cpuarray == NULL)
			die("json_array() failed");

		first_time = 1;
	}

	JSON_Value *valcpu = json_value_init_object();
	if (valcpu == NULL)
		die("json_value_init_object() failed");

	JSON_Object *cpu = json_object(valcpu);
	if (cpu == NULL)
		die("json_object() failed");

	if (json_object_set_number(cpu, "index", index) != 0)
		die("json_object_set_number() failed");

	if (json_object_set_number(cpu, "phyid", phyid) != 0)
		die("json_object_set_number() failed");

	if (json_array_append_value(cpuarray, valcpu) != 0)
		die("json_array_append_value() failed");

	if (first_time) {
		JSON_Value *value = json_array_get_wrapping_value(cpuarray);
		if (value == NULL)
			die("json_array_get_wrapping_value() failed");

		if (json_object_set_value(meta, "cpus", value) != 0)
			die("json_object_set_value failed");
	}
}

static void
proc_set_app(int appid)
{
	JSON_Object *meta = json_value_get_object(rproc.meta);

	if (meta == NULL)
		die("json_value_get_object failed");

	if (json_object_set_number(meta, "app_id", appid) != 0)
		die("json_object_set_number for app_id failed");
}

static void
proc_set_version(void)
{
	JSON_Object *meta = json_value_get_object(rproc.meta);

	if (meta == NULL)
		die("json_value_get_object failed");

	if (json_object_set_number(meta, "version", OVNI_METADATA_VERSION) != 0)
		die("json_object_set_number for version failed");

	if (json_object_set_string(meta, "model_version", OVNI_MODEL_VERSION) != 0)
		die("json_object_set_string for model_version failed");
}


void
ovni_proc_set_rank(int rank, int nranks)
{
	if (!rproc.ready)
		die("process not yet initialized");

	JSON_Object *meta = json_value_get_object(rproc.meta);

	if (meta == NULL)
		die("json_value_get_object failed");

	if (json_object_set_number(meta, "rank", rank) != 0)
		die("json_object_set_number for rank failed");

	if (json_object_set_number(meta, "nranks", nranks) != 0)
		die("json_object_set_number for nranks failed");
}

/* Create $tracedir/loom.$loom/proc.$pid and return it in path. */
static void
mkdir_proc(char *path, const char *tracedir, const char *loom, int pid)
{
	snprintf(path, PATH_MAX, "%s", tracedir);

	/* May fail if another loom created the directory already */
	mkdir(path, 0755);

	snprintf(path, PATH_MAX, "%s/loom.%s", tracedir, loom);

	/* Also may fail */
	mkdir(path, 0755);

	snprintf(path, PATH_MAX, "%s/loom.%s/proc.%d", tracedir, loom, pid);

	/* But this one shall not fail */
	if (mkdir(path, 0755))
		die("mkdir %s failed:", path);
}

static void
create_proc_dir(const char *loom, int pid)
{
	char *tmpdir = getenv("OVNI_TMPDIR");

	if (tmpdir != NULL) {
		rproc.move_to_final = 1;
		mkdir_proc(rproc.procdir, tmpdir, loom, pid);
		mkdir_proc(rproc.procdir_final, OVNI_TRACEDIR, loom, pid);
	} else {
		rproc.move_to_final = 0;
		mkdir_proc(rproc.procdir, OVNI_TRACEDIR, loom, pid);
	}
}

void
ovni_proc_init(int app, const char *loom, int pid)
{
	if (rproc.ready)
		die("pid %d already initialized", pid);

	memset(&rproc, 0, sizeof(rproc));

	if (strlen(loom) >= OVNI_MAX_HOSTNAME)
		die("loom name too long: %s", loom);

	strcpy(rproc.loom, loom);
	rproc.pid = pid;
	rproc.app = app;
	rproc.clockid = CLOCK_MONOTONIC;

	create_proc_dir(loom, pid);

	proc_metadata_init(&rproc);

	rproc.ready = 1;

	proc_set_version();
	proc_set_app(app);
}

static int
move_thread_to_final(const char *src, const char *dst)
{
	char buffer[1024];

	FILE *infile = fopen(src, "r");

	if (infile == NULL) {
		err("fopen(%s) failed:", src);
		return -1;
	}

	FILE *outfile = fopen(dst, "w");

	if (outfile == NULL) {
		err("fopen(%s) failed:", src);
		return -1;
	}

	size_t bytes;
	while ((bytes = fread(buffer, 1, sizeof(buffer), infile)) > 0)
		fwrite(buffer, 1, bytes, outfile);

	fclose(outfile);
	fclose(infile);

	if (remove(src) != 0) {
		err("remove(%s) failed:", src);
		return -1;
	}

	return 0;
}

static void
move_procdir_to_final(const char *procdir, const char *procdir_final)
{
	DIR *dir;
	int ret = 0;

	if ((dir = opendir(procdir)) == NULL) {
		err("opendir %s failed:", procdir);
		return;
	}

	struct dirent *dirent;
	const char *prefix = "thread.";
	while ((dirent = readdir(dir)) != NULL) {
		/* It should only contain thread.* directories, skip others */
		if (strncmp(dirent->d_name, prefix, strlen(prefix)) != 0)
			continue;

		char thread[PATH_MAX];
		if (snprintf(thread, PATH_MAX, "%s/%s", procdir,
				    dirent->d_name)
				>= PATH_MAX) {
			err("snprintf: path too large: %s/%s", procdir,
					dirent->d_name);
			ret = 1;
			continue;
		}

		char thread_final[PATH_MAX];
		if (snprintf(thread_final, PATH_MAX, "%s/%s", procdir_final,
				    dirent->d_name)
				>= PATH_MAX) {
			err("snprintf: path too large: %s/%s", procdir_final,
					dirent->d_name);
			ret = 1;
			continue;
		}

		if (move_thread_to_final(thread, thread_final) != 0)
			ret = 1;
	}

	closedir(dir);

	if (rmdir(procdir) != 0) {
		err("rmdir(%s) failed:", procdir);
		ret = 1;
	}

	/* Warn the user, but we cannot do much at this point */
	if (ret)
		err("errors occurred when moving the trace to %s", procdir_final);
}

void
ovni_proc_fini(void)
{
	if (!rproc.ready)
		die("process not initialized");

	/* Mark the process no longer ready */
	rproc.ready = 0;

	if (rproc.move_to_final) {
		proc_metadata_store(rproc.meta, rproc.procdir_final);
		move_procdir_to_final(rproc.procdir, rproc.procdir_final);
	} else {
		proc_metadata_store(rproc.meta, rproc.procdir);
	}
}

static void
write_evbuf(uint8_t *buf, size_t size)
{
	do {
		ssize_t written = write(rthread.streamfd, buf, size);

		if (written < 0)
			die("failed to write buffer to disk:");

		size -= written;
		buf += written;
	} while (size > 0);
}

static void
flush_evbuf(void)
{
	write_evbuf(rthread.evbuf, rthread.evlen);

	rthread.evlen = 0;
}

static void
write_stream_header(void)
{
	struct ovni_stream_header *h =
			(struct ovni_stream_header *) rthread.evbuf;

	memcpy(h->magic, OVNI_STREAM_MAGIC, 4);
	h->version = OVNI_STREAM_VERSION;

	rthread.evlen = sizeof(struct ovni_stream_header);
	flush_evbuf();
}

void
ovni_thread_init(pid_t tid)
{
	if (rthread.ready) {
		warn("thread %d already initialized, ignored", tid);
		return;
	}

	if (tid == 0)
		die("cannot use tid=%d", tid);

	if (!rproc.ready)
		die("process not yet initialized");

	memset(&rthread, 0, sizeof(rthread));

	rthread.tid = tid;
	rthread.evlen = 0;
	rthread.evbuf = malloc(OVNI_MAX_EV_BUF);

	if (rthread.evbuf == NULL)
		die("malloc failed:");

	create_trace_stream();
	write_stream_header();

	rthread.ready = 1;
}

void
ovni_thread_free(void)
{
	if (!rthread.ready)
		die("thread not initialized");

	free(rthread.evbuf);
}

int
ovni_thread_isready(void)
{
	return rthread.ready;
}

#ifdef USE_TSC
static inline uint64_t
clock_tsc_now(void)
{
	uint32_t lo, hi;

	/* RDTSC copies contents of 64-bit TSC into EDX:EAX */
	__asm__ volatile("rdtsc"
			 : "=a"(lo), "=d"(hi));
	return (uint64_t) hi << 32 | lo;
}
#endif

static uint64_t
clock_monotonic_now(void)
{
	uint64_t ns = 1000ULL * 1000ULL * 1000ULL;
	struct timespec tp;

	if (clock_gettime(rproc.clockid, &tp))
		die("clock_gettime() failed:");

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
	if (ev->header.flags & OVNI_EV_JUMBO)
		return get_jumbo_payload_size(ev);

	int size = ev->header.flags & 0x0f;

	if (size == 0)
		return 0;

	/* The minimum size is 2 bytes, so we can encode a length of 16
	 * bytes using 4 bits (0x0f) */
	size++;

	return size;
}

void
ovni_payload_add(struct ovni_ev *ev, const uint8_t *buf, int size)
{
	if (ev->header.flags & OVNI_EV_JUMBO)
		die("event is marked as jumbo");

	if (size < 2)
		die("payload size %d too small", size);

	size_t payload_size = ovni_payload_size(ev);

	/* Ensure we have room */
	if (payload_size + size > sizeof(ev->payload))
		die("no space left for %d bytes", size);

	memcpy(&ev->payload.u8[payload_size], buf, size);
	payload_size += size;

	ev->header.flags = (ev->header.flags & 0xf0) | ((payload_size - 1) & 0x0f);
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
	struct ovni_ev pre = {0}, post = {0};

	if (!rthread.ready)
		die("thread is not initialized");

	if (!rproc.ready)
		die("process is not initialized");

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
	struct ovni_ev pre = {0}, post = {0};

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

	if (ovni_payload_size(ev) != 0)
		die("the event payload must be empty");

	ovni_payload_add(ev, (uint8_t *) &bufsize, sizeof(bufsize));
	size_t evsize = ovni_ev_size(ev);

	size_t totalsize = evsize + bufsize;

	if (totalsize >= OVNI_MAX_EV_BUF)
		die("event too large");

	/* Check if the event fits or flush first otherwise */
	if (rthread.evlen + totalsize >= OVNI_MAX_EV_BUF) {
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

	if (flushed) {
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
	if (rthread.evlen + size >= OVNI_MAX_EV_BUF) {
		/* Measure the flush times */
		t0 = ovni_clock_now();
		flush_evbuf();
		t1 = ovni_clock_now();
		flushed = 1;
	}

	memcpy(&rthread.evbuf[rthread.evlen], ev, size);
	rthread.evlen += size;

	if (flushed) {
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
