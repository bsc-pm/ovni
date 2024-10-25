/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: MIT */

#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
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
#include "utlist.h"

enum {
	ST_UNINIT = 0,
	ST_INIT,
	ST_READY,
	ST_GONE,
};

struct ovni_rcpu {
	int index;
	int phyid;
	struct ovni_rcpu *next;
	struct ovni_rcpu *prev;
};

/* State of each thread on runtime */
struct ovni_rthread {
	/* Current thread id */
	pid_t tid;

	/* Stream trace file descriptor */
	int streamfd;

	int ready;
	int finished;

	/* The number of bytes filled with events */
	size_t evlen;

	/* Buffer to write events */
	uint8_t *evbuf;

	struct ovni_rcpu *cpus;

	int rank_set;
	int rank;
	int nranks;

	/* Where the stream dir is finally copied */
	char thdir_final[PATH_MAX];
	char thdir[PATH_MAX];

	JSON_Value *meta;
};

/* State of each process on runtime */
struct ovni_rproc {
	/* Where the process trace is finally copied */
	char procdir_final[PATH_MAX];

	/* Where the process trace is flushed */
	char procdir[PATH_MAX];
	char loomdir[PATH_MAX];
	char tmpdir[PATH_MAX];

	/* If needs to be moved at the end */
	int move_to_final;

	int app;
	int pid;
	char loom[OVNI_MAX_HOSTNAME];
	clockid_t clockid;

	atomic_int st;

	JSON_Value *meta;
};

/* Data per process */
struct ovni_rproc rproc = {0};

/* Data per thread */
_Thread_local struct ovni_rthread rthread = {0};

void
ovni_version_get(const char **version, const char **commit)
{
	*version = OVNI_LIB_VERSION;
	*commit = OVNI_GIT_COMMIT;
}

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
		die("incompatible ovni major version: wants %s, got %s",
				version, OVNI_LIB_VERSION);
	}

	/* Only fail if the minor is newer */
	if (provided[1] > expected[1]) {
		die("incompatible ovni minor version: wants %s, got %s",
				version, OVNI_LIB_VERSION);
	}

	/* Ignore the patch number */
}

/* Create dir $procdir/thread.$tid and return it in path. */
static void
mkdir_thread(char *path, const char *procdir, int tid)
{
	if (snprintf(path, PATH_MAX, "%s/thread.%d",
				procdir, tid) >= PATH_MAX) {
		die("path too long: %s/thread.%d", procdir, tid);
	}

	if (mkpath(path, 0755, /* subdir */ 1))
		die("mkpath %s failed:", path);
}

static void
create_thread_dir(int tid)
{
	/* The procdir must have been created earlier */
	mkdir_thread(rthread.thdir, rproc.procdir, tid);
	if (rproc.move_to_final)
		mkdir_thread(rthread.thdir_final, rproc.procdir_final, tid);
}

static void
create_trace_stream(void)
{
	char path[PATH_MAX];

	int written = snprintf(path, PATH_MAX, "%s/thread.%d/stream.obs",
			rproc.procdir, rthread.tid);

	if (written >= PATH_MAX) {
		die("path too long: %s/thread.%d/stream.obs",
				rproc.procdir, rthread.tid);
	}

	rthread.streamfd = open(path, O_WRONLY | O_CREAT, 0644);

	if (rthread.streamfd == -1)
		die("open %s failed:", path);
}

void
ovni_add_cpu(int index, int phyid)
{
	if (index < 0)
		die("cannot use negative index %d", index);

	if (phyid < 0)
		die("cannot use negative CPU id %d", phyid);

	if (atomic_load(&rproc.st) != ST_READY)
		die("process not ready");

	if (!rthread.ready)
		die("thread not yet initialized");

	struct ovni_rcpu *cpu = malloc(sizeof(*cpu));
	if (cpu == NULL)
		die("malloc failed:");

	cpu->index = index;
	cpu->phyid = phyid;

	DL_APPEND(rthread.cpus, cpu);
}

void
ovni_proc_set_rank(int rank, int nranks)
{
	if (atomic_load(&rproc.st) != ST_READY)
		die("process not ready");

	if (!rthread.ready)
		die("thread not yet initialized");

	rthread.rank_set = 1;
	rthread.rank = rank;
	rthread.nranks = nranks;
}

/* Create $tracedir/loom.$loom/proc.$pid and return it in path. */
static void
mkdir_proc(char *path, const char *tracedir, const char *loom, int pid)
{
	snprintf(path, PATH_MAX, "%s/loom.%s/proc.%d/", tracedir, loom, pid);

	/* But this one shall not fail */
	if (mkpath(path, 0755, /* subdir */ 1))
		die("mkdir %s failed:", path);
}

static void
create_proc_dir(const char *loom, int pid)
{
	char *tmpdir = getenv("OVNI_TMPDIR");
	char *tracedir = getenv("OVNI_TRACEDIR");

	/* Use default tracedir if user did not request any */
	if (tracedir == NULL)
		tracedir = OVNI_TRACEDIR;

	if (snprintf(rproc.loomdir, PATH_MAX, "%s/loom.%s", tmpdir, loom) >= PATH_MAX)
		die("loom path too long: %s/loom.%s", tmpdir, loom);

	if (tmpdir != NULL) {
		if (snprintf(rproc.tmpdir, PATH_MAX, "%s", tmpdir) >= PATH_MAX)
			die("tmpdir path too long: %s", tmpdir);
		rproc.move_to_final = 1;
		mkdir_proc(rproc.procdir, tmpdir, loom, pid);
		mkdir_proc(rproc.procdir_final, tracedir, loom, pid);
	} else {
		rproc.move_to_final = 0;
		mkdir_proc(rproc.procdir, tracedir, loom, pid);
	}
}

void
ovni_proc_init(int app, const char *loom, int pid)
{
	/* Protect against two threads calling at the same time */
	int st = ST_UNINIT;
	bool was_uninit = atomic_compare_exchange_strong(&rproc.st,
			&st, ST_INIT);

	if (!was_uninit) {
		if (st == ST_INIT)
			die("pid %d already being initialized", pid);
		else if (st == ST_READY)
			die("pid %d already initialized", pid);
		else if (st == ST_GONE)
			die("pid %d has finished, cannot init again", pid);
	}

	if (strlen(loom) >= OVNI_MAX_HOSTNAME)
		die("loom name too long: %s", loom);

	strcpy(rproc.loom, loom);
	rproc.pid = pid;
	rproc.app = app;
	rproc.clockid = CLOCK_MONOTONIC;

	create_proc_dir(loom, pid);

	atomic_store(&rproc.st, ST_READY);
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
move_thdir_to_final(const char *thdir, const char *thdir_final)
{
	DIR *dir;
	int ret = 0;

	if ((dir = opendir(thdir)) == NULL) {
		err("opendir %s failed:", thdir);
		return;
	}

	struct dirent *dirent;
	const char *prefix = "stream.";
	while ((dirent = readdir(dir)) != NULL) {
		/* It should only contain stream.* directories, skip others */
		if (strncmp(dirent->d_name, prefix, strlen(prefix)) != 0)
			continue;

		char thread[PATH_MAX];
		if (snprintf(thread, PATH_MAX, "%s/%s", thdir,
				    dirent->d_name)
				>= PATH_MAX) {
			err("snprintf: path too large: %s/%s", thdir,
					dirent->d_name);
			ret = 1;
			continue;
		}

		char thread_final[PATH_MAX];
		if (snprintf(thread_final, PATH_MAX, "%s/%s", thdir_final,
				    dirent->d_name)
				>= PATH_MAX) {
			err("snprintf: path too large: %s/%s", thdir_final,
					dirent->d_name);
			ret = 1;
			continue;
		}

		if (move_thread_to_final(thread, thread_final) != 0)
			ret = 1;
	}

	closedir(dir);

	/* Warn the user, but we cannot do much at this point */
	if (ret)
		err("errors occurred when moving the thread dir to %s", thdir_final);
}

static void
try_clean_dir(const char *dir)
{
	/* Only warn if we find an unexpected error */
	if (rmdir(dir) != 0 && errno != ENOTEMPTY && errno != ENOENT)
		warn("cannot remove dir %s:", dir);
}

void
ovni_proc_fini(void)
{
	/* Protect against two threads calling at the same time */
	int st = ST_READY;
	bool was_ready = atomic_compare_exchange_strong(&rproc.st,
			&st, ST_GONE);

	if (!was_ready)
		die("process not ready");

	if (rproc.move_to_final) {
		try_clean_dir(rproc.procdir);
		try_clean_dir(rproc.loomdir);
		try_clean_dir(rproc.tmpdir);
	}
}

static void
write_evbuf(uint8_t *buf, size_t size)
{
	do {
		ssize_t written = write(rthread.streamfd, buf, size);

		if (written < 0)
			die("failed to write buffer to disk:");

		size -= (size_t) written;
		buf += (size_t) written;
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

static void
thread_metadata_store(void)
{
	char path[PATH_MAX];
	int written = snprintf(path, PATH_MAX, "%s/thread.%d/stream.json",
			rproc.procdir, rthread.tid);

	if (written >= PATH_MAX)
		die("thread trace path too long: %s/thread.%d/stream.json",
				rproc.procdir, rthread.tid);

	if (json_serialize_to_file_pretty(rthread.meta, path) != JSONSuccess)
		die("failed to write thread metadata");
}

void
ovni_thread_require(const char *model, const char *version)
{
	if (!rthread.ready)
		die("thread not initialized");

	/* Sanitize model */
	if (model == NULL)
		die("model string is NULL");

	if (strpbrk(model, " .") != NULL)
		die("malformed model name");

	if (strlen(model) <= 1)
		die("model name must have more than 1 character");

	/* Sanitize version */
	if (version == NULL)
		die("version string is NULL");

	int parsedver[3];
	if (version_parse(version, parsedver) != 0)
		die("failed to parse provided version \"%s\"", version);

	/* Store into metadata */
	JSON_Object *meta = json_value_get_object(rthread.meta);

	if (meta == NULL)
		die("json_value_get_object failed");

	char dotpath[128];
	if (snprintf(dotpath, 128, "ovni.require.%s", model) >= 128)
		die("model name too long");

	if (json_object_dotset_string(meta, dotpath, version) != 0)
		die("json_object_dotset_string failed");
}

static void
thread_metadata_populate(void)
{
	JSON_Object *meta = json_value_get_object(rthread.meta);

	if (meta == NULL)
		die("json_value_get_object failed");

	if (json_object_dotset_number(meta, "version", OVNI_METADATA_VERSION) != 0)
		die("json_object_dotset_string failed");

	if (json_object_dotset_string(meta, "ovni.lib.version", OVNI_LIB_VERSION) != 0)
		die("json_object_dotset_string failed");

	if (json_object_dotset_string(meta, "ovni.lib.commit", OVNI_GIT_COMMIT) != 0)
		die("json_object_dotset_string failed");

	if (json_object_dotset_string(meta, "ovni.part", "thread") != 0)
		die("json_object_dotset_string failed");

	if (json_object_dotset_number(meta, "ovni.tid", (double) rthread.tid) != 0)
		die("json_object_dotset_number failed");

	if (json_object_dotset_number(meta, "ovni.pid", (double) rproc.pid) != 0)
		die("json_object_dotset_number failed");

	if (json_object_dotset_string(meta, "ovni.loom", rproc.loom) != 0)
		die("json_object_dotset_string failed");

	if (json_object_dotset_number(meta, "ovni.app_id", rproc.app) != 0)
		die("json_object_dotset_number for ovni.app_id failed");
}

static void
thread_metadata_init(void)
{
	rthread.meta = json_value_init_object();

	if (rthread.meta == NULL)
		die("failed to create metadata JSON object");

	thread_metadata_populate();

	/* Store initial metadata on disk, to detect broken streams */
	thread_metadata_store();
}

void
ovni_thread_init(pid_t tid)
{
	if (rthread.ready) {
		warn("thread %d already initialized, ignored", tid);
		return;
	}

	if (rthread.finished)
		die("thread %d has finished, cannot init again", tid);

	if (tid == 0)
		die("cannot use tid=%d", tid);

	if (atomic_load(&rproc.st) != ST_READY)
		die("process not ready");

	memset(&rthread, 0, sizeof(rthread));

	rthread.tid = tid;
	rthread.evlen = 0;
	rthread.evbuf = malloc(OVNI_MAX_EV_BUF);

	if (rthread.evbuf == NULL)
		die("malloc failed:");

	create_thread_dir(tid);
	create_trace_stream();
	write_stream_header();

	thread_metadata_init();

	rthread.ready = 1;

	ovni_thread_require("ovni", OVNI_MODEL_VERSION);
}

static void
set_thread_rank(JSON_Object *meta)
{
	if (json_object_dotset_number(meta, "ovni.rank", rthread.rank) != 0)
		die("json_object_set_number for rank failed");

	if (json_object_dotset_number(meta, "ovni.nranks", rthread.nranks) != 0)
		die("json_object_set_number for nranks failed");
}

static void
set_thread_cpus(JSON_Object *meta)
{
	JSON_Value *value = json_value_init_array();
	if (value == NULL)
		die("json_value_init_array() failed");

	JSON_Array *cpuarray = json_array(value);
	if (cpuarray == NULL)
		die("json_array() failed");

	for (struct ovni_rcpu *c = rthread.cpus; c; c = c->next) {
		JSON_Value *valcpu = json_value_init_object();
		if (valcpu == NULL)
			die("json_value_init_object() failed");

		JSON_Object *cpu = json_object(valcpu);
		if (cpu == NULL)
			die("json_object() failed");

		if (json_object_set_number(cpu, "index", c->index) != 0)
			die("json_object_set_number() failed");

		if (json_object_set_number(cpu, "phyid", c->phyid) != 0)
			die("json_object_set_number() failed");

		if (json_array_append_value(cpuarray, valcpu) != 0)
			die("json_array_append_value() failed");
	}

	if (json_object_dotset_value(meta, "ovni.loom_cpus", value) != 0)
		die("json_object_dotset_value failed");
}

void
ovni_thread_free(void)
{
	if (rthread.finished)
		die("thread already finished");

	if (!rthread.ready)
		die("thread not initialized");

	JSON_Object *meta = json_value_get_object(rthread.meta);

	if (meta == NULL)
		die("json_value_get_object failed");

	if (rthread.rank_set)
		set_thread_rank(meta);

	/* It can happen there are no CPUs defined if there is another
	 * process in the loom that defines them. */
	if (rthread.cpus)
		set_thread_cpus(meta);

	/* Mark it finished so we can detect partial streams */
	if (json_object_dotset_number(meta, "ovni.finished", 1) != 0)
		die("json_object_dotset_string failed");

	thread_metadata_store();

	free(rthread.evbuf);
	rthread.evbuf = NULL;

	close(rthread.streamfd);
	rthread.streamfd = -1;

	if (rproc.move_to_final) {
		/* The dir rthread.thdir_final must exist in the FS */
		move_thdir_to_final(rthread.thdir, rthread.thdir_final);
		try_clean_dir(rthread.thdir);
	}

	rthread.finished = 1;
	rthread.ready = 0;
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

	return (uint64_t) tp.tv_sec * ns + (uint64_t) tp.tv_nsec;
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
	ev->header.model = (uint8_t) mcv[0];
	ev->header.category = (uint8_t) mcv[1];
	ev->header.value = (uint8_t) mcv[2];
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
		return (int) get_jumbo_payload_size(ev);

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

	size_t payload_size = (size_t) ovni_payload_size(ev);

	/* Ensure we have room */
	if (payload_size + (size_t) size > sizeof(ev->payload))
		die("no space left for %d bytes", size);

	memcpy(&ev->payload.u8[payload_size], buf, (size_t) size);
	payload_size += (size_t) size;

	ev->header.flags = (uint8_t) ((ev->header.flags & 0xf0)
			| ((payload_size - 1) & 0x0f));
}

int
ovni_ev_size(const struct ovni_ev *ev)
{
	return (int) sizeof(ev->header) + ovni_payload_size(ev);
}

static void
ovni_ev_add(struct ovni_ev *ev);

void
ovni_flush(void)
{
	struct ovni_ev pre = {0}, post = {0};

	if (!rthread.ready)
		die("thread is not initialized");

	if (atomic_load(&rproc.st) != ST_READY)
		die("process not ready");

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
	if (!rthread.ready)
		die("thread is not initialized");

	int flushed = 0;
	uint64_t t0, t1;

	if (ovni_payload_size(ev) != 0)
		die("the event payload must be empty");

	ovni_payload_add(ev, (uint8_t *) &bufsize, sizeof(bufsize));
	size_t evsize = (size_t) ovni_ev_size(ev);

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
	if (!rthread.ready)
		die("thread is not initialized");

	int flushed = 0;
	uint64_t t0, t1;

	size_t size = (size_t) ovni_ev_size(ev);

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

/* Attributes */

static JSON_Object *
get_thread_metadata(void)
{
	if (rthread.finished)
		die("thread already finished");

	if (!rthread.ready)
		die("thread not initialized");

	JSON_Object *meta = json_value_get_object(rthread.meta);

	if (meta == NULL)
		die("json_value_get_object failed");

	return meta;
}

/**
 * Determines if the key exists in the metadata.
 *
 * @returns 1 if the key exists, 0 otherwise.
 */
int
ovni_attr_has(const char *key)
{
	JSON_Object *obj = get_thread_metadata();
	JSON_Value *val = json_object_dotget_value(obj, key);

	if (val == NULL)
		return 0;

	return 1;
}

/**
 * Stores a double attribute.
 *
 * @param key The key of the attribute as a dot path.
 * @param num The double value to be stored.
 */
void
ovni_attr_set_double(const char *key, double num)
{
	JSON_Object *obj = get_thread_metadata();

	if (json_object_dotset_number(obj, key, num) != 0)
		die("json_object_dotset_number() failed");
}

/**
 * Retrieves a double attribute.
 *
 * @param key The key of the attribute as a dot path.
 *
 * @return The double on the key. If there is any problem it aborts.
 */
double
ovni_attr_get_double(const char *key)
{
	JSON_Object *obj = get_thread_metadata();
	JSON_Value *val = json_object_dotget_value(obj, key);
	if (val == NULL)
		die("key not found: %s", key);

	if (json_value_get_type(val) != JSONNumber)
		die("value with key '%s' is not a number", key);

	return json_value_get_number(val);
}

/**
 * Retrieves a boolean attribute.
 *
 * @param key The key of the attribute as a dot path.
 *
 * @return The boolean on the key. If there is any problem it aborts.
 */
int
ovni_attr_get_boolean(const char *key)
{
	JSON_Object *obj = get_thread_metadata();
	JSON_Value *val = json_object_dotget_value(obj, key);
	if (val == NULL)
		die("key not found: %s", key);

	if (json_value_get_type(val) != JSONBoolean)
		die("value with key '%s' is not a boolean", key);

	return json_value_get_boolean(val);
}

/**
 * Stores a boolean attribute.
 *
 * @param key The key of the attribute as a dot path.
 * @param num The boolean value to be stored.
 */
void
ovni_attr_set_boolean(const char *key, int value)
{
	JSON_Object *obj = get_thread_metadata();

	if (json_object_dotset_boolean(obj, key, value) != 0)
		die("json_object_dotset_boolean() failed");
}

/**
 * Stores a string attribute.
 *
 * @param key The key of the attribute as a dot path.
 * @param str The string value to be stored. It will be internally duplicated,
 * so it can be free on return.
 */
void
ovni_attr_set_str(const char *key, const char *value)
{
	JSON_Object *obj = get_thread_metadata();

	if (json_object_dotset_string(obj, key, value) != 0)
		die("json_object_dotset_string() failed");
}

/**
 * Retrieves a string attribute from a key.
 * If not found or if there is any problem it aborts before returning.
 *
 * @param key The key of the attribute as a dot path.
 *
 * @return A pointer to a not-NULL read-only string value for the given key.
 */
const char *
ovni_attr_get_str(const char *key)
{
	JSON_Object *obj = get_thread_metadata();
	JSON_Value *val = json_object_dotget_value(obj, key);
	if (val == NULL)
		die("key not found: %s", key);

	if (json_value_get_type(val) != JSONString)
		die("value with key '%s' is not a string", key);

	return json_value_get_string(val);
}

/**
 * Stores a JSON value into an attribute.
 *
 * @param key The key of the attribute as a dot path.
 * @param json The JSON value to be stored.
 *
 * The value specified as a JSON string can be of any type (dictionary, array,
 * string, double...) as long as it is valid JSON. Any errors in parsing the
 * JSON string or in storing the resulting value will abort the program.
 */
void
ovni_attr_set_json(const char *key, const char *json)
{
	JSON_Object *obj = get_thread_metadata();
	JSON_Value *val = json_parse_string(json);
	if (val == NULL)
		die("cannot parse json: %s", json);

	if (json_object_dotset_value(obj, key, val) != 0)
		die("json_object_dotset_value() failed");
}


/**
 * Serializes a JSON value into a string.
 *
 * @param key The key of the attribute as a dot path.
 *
 * @resurn A zero-terminated string containing the serialized JSON
 * representation of the provided key. This string is allocated with malloc()
 * and it is reponsability of the user to liberate the memory with free().
 *
 * Any errors will abort the program.
 */
char *
ovni_attr_get_json(const char *key)
{
	JSON_Object *obj = get_thread_metadata();
	JSON_Value *val = json_object_dotget_value(obj, key);
	if (val == NULL)
		die("key not found: %s", key);

	char *str = json_serialize_to_string(val);
	if (str == NULL)
		die("json_serialize_to_string() failed");

	return str;
}


/**
 * Writes the metadata attributes to disk.
 * Only used to ensure they are not lost in a crash. They are already written in
 * ovni_thread_free().
 */
void
ovni_attr_flush(void)
{
	if (rthread.finished)
		die("thread already finished");

	if (!rthread.ready)
		die("thread not initialized");

	thread_metadata_store();
}

/* Mark API */

/**
 * Creates a new mark type.
 *
 * @param type The mark type that must be in the range 0 to 99, both included.
 * @param flags An OR of OVNI_MARK_* flags.
 * @param title The title that will be displayed in Paraver.
 *
 * It can be called from multiple threads as long as they all use the same
 * arguments. Only one thread in all nodes needs to call it to define a type.
 */
void
ovni_mark_type(int32_t type, long flags, const char *title)
{
	if (type < 0 || type >= 100)
		die("type must be in [0,100) range");

	if (!title || title[0] == '\0')
		die("bad title");

	JSON_Object *meta = get_thread_metadata();

	char key[128];
	if (snprintf(key, 128, "ovni.mark.%"PRId32, type) >= 128)
		die("type key too long");

	JSON_Value *val = json_object_dotget_value(meta, key);
	if (val != NULL)
		die("type %"PRId32" already defined", type);

	if (snprintf(key, 128, "ovni.mark.%"PRId32".title", type) >= 128)
		die("title key too long");

	if (json_object_dotset_string(meta, key, title) != 0)
		die("json_object_dotset_string() failed for title");

	const char *chan_type = flags & OVNI_MARK_STACK ? "stack" : "single";
	if (snprintf(key, 128, "ovni.mark.%"PRId32".chan_type", type) >= 128)
		die("chan_type key too long");

	if (json_object_dotset_string(meta, key, chan_type) != 0)
		die("json_object_dotset_string() failed for chan_type");
}

/**
 * Defines a label for the given value.
 *
 * @param type The mark type.
 * @param type The numeric value to which assign a label. The value 0 is
 * forbidden.
 * @param label The label that will be displayed in Paraver.
 *
 * It only needs to be called once from a thread to globally assign a label to a
 * given value. It can be called from multiple threads as long as the value for
 * a given type has only one unique label. Multiple calls with the same
 * arguments are valid, but with only a distinct label are not.
 */
void
ovni_mark_label(int32_t type, int64_t value, const char *label)
{
	if (type < 0 || type >= 100)
		die("type must be in [0,100) range");

	if (value <= 0)
		die("value must be >0");

	if (!label || label[0] == '\0')
		die("bad label");

	JSON_Object *meta = get_thread_metadata();

	char key[128];
	if (snprintf(key, 128, "ovni.mark.%"PRId32, type) >= 128)
		die("type key too long");

	JSON_Value *valtype = json_object_dotget_value(meta, key);
	if (valtype == NULL)
		die("type %"PRId32" not defined", type);

	if (snprintf(key, 128, "ovni.mark.%"PRId32".labels.%"PRId64, type, value) >= 128)
		die("value key too long");

	JSON_Value *val = json_object_dotget_value(meta, key);
	if (val != NULL)
		die("label '%s' already defined", label);

	if (json_object_dotset_string(meta, key, label) != 0)
		die("json_object_dotset_string() failed");
}

/**
 * Pushes a value into a stacked mark channel.
 *
 * @param type The mark type which must be defined with the OVNI_MARK_STACK flag.
 * @param value The value to be pushed, The value 0 is forbidden.
 */
void
ovni_mark_push(int32_t type, int64_t value)
{
	if (value == 0)
		die("value cannot be 0, type %ld", (long) type);

	struct ovni_ev ev = {0};
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_set_mcv(&ev, "OM[");
	ovni_payload_add(&ev, (uint8_t *) &value, sizeof(value));
	ovni_payload_add(&ev, (uint8_t *) &type, sizeof(type));
	ovni_ev_add(&ev);
}

/**
 * Pops a value from a stacked mark channel.
 *
 * @param type The mark type which must be defined with the OVNI_MARK_STACK flag.
 * @param value The value to be popped, which must match the current value. The
 * value 0 is forbidden.
 */
void
ovni_mark_pop(int32_t type, int64_t value)
{
	if (value == 0)
		die("value cannot be 0, type %ld", (long) type);

	struct ovni_ev ev = {0};
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_set_mcv(&ev, "OM]");
	ovni_payload_add(&ev, (uint8_t *) &value, sizeof(value));
	ovni_payload_add(&ev, (uint8_t *) &type, sizeof(type));
	ovni_ev_add(&ev);
}

/**
 * Sets the value to a normal mark channel.
 *
 * @param type The mark type which must be defined without the OVNI_MARK_STACK flag.
 * @param value The value to be set. The value 0 is forbidden.
 */
void
ovni_mark_set(int32_t type, int64_t value)
{
	if (value == 0)
		die("value cannot be 0, type %ld", (long) type);

	struct ovni_ev ev = {0};
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_set_mcv(&ev, "OM=");
	ovni_payload_add(&ev, (uint8_t *) &value, sizeof(value));
	ovni_payload_add(&ev, (uint8_t *) &type, sizeof(type));
	ovni_ev_add(&ev);
}
