/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "system.h"
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "emu_args.h"
#include "loom.h"
#include "metadata.h"
#include "proc.h"
#include "pv/prf.h"
#include "pv/pvt.h"
#include "recorder.h"
#include "stream.h"
#include "thread.h"
#include "trace.h"
#include "uthash.h"
#include "utlist.h"
struct bay;

static struct thread *
create_thread(struct proc *proc, const char *tracedir, const char *relpath)
{
	int tid;
	if (thread_relpath_get_tid(relpath, &tid) != 0) {
		err("cannot get thread tid from %s", relpath);
		return NULL;
	}

	struct thread *thread = proc_find_thread(proc, tid);

	if (thread != NULL) {
		err("thread with tid %d already exists", tid);
		return NULL;
	}

	thread = malloc(sizeof(struct thread));
	if (thread == NULL) {
		err("malloc failed:");
		return NULL;
	}

	if (thread_init_begin(thread, relpath) != 0) {
		err("cannot init thread");
		return NULL;
	}

	/* Build metadata path */
	char mpath[PATH_MAX];
	if (snprintf(mpath, PATH_MAX, "%s/%s/thread.%d.json",
				tracedir, proc->id, tid) >= PATH_MAX) {
		err("path too long");
		return NULL;
	}

	if (metadata_load_thread(mpath, thread) != 0) {
		err("cannot load metadata from %s", mpath);
		return NULL;
	}

	if (proc_add_thread(proc, thread) != 0) {
		err("failed to add thread to proc");
		return NULL;
	}

	return thread;
}

static struct proc *
create_proc(struct loom *loom, const char *tracedir, const char *relpath)
{
	int pid;
	if (proc_relpath_get_pid(relpath, &pid) != 0) {
		err("cannot get proc pid from %s", relpath);
		return NULL;
	}

	struct proc *proc = loom_find_proc(loom, pid);

	if (proc != NULL)
		return proc;

	proc = malloc(sizeof(struct proc));

	if (proc == NULL) {
		err("malloc failed:");
		return NULL;
	}

	if (proc_init_begin(proc, relpath) != 0) {
		err("proc_init_begin failed");
		return NULL;
	}

	/* Build metadata path */
	char mpath[PATH_MAX];

	if (snprintf(mpath, PATH_MAX, "%s/%s/metadata.json",
				tracedir, proc->id) >= PATH_MAX) {
		err("path too long");
		return NULL;
	}

	/* Load metadata too */
	if (metadata_load_proc(mpath, loom, proc) != 0) {
		err("cannot load metadata from %s", mpath);
		return NULL;
	}

	if (loom_add_proc(loom, proc) != 0) {
		err("loom_add_proc failed");
		return NULL;
	}

	return proc;
}

static struct loom *
find_loom(struct system *sys, const char *id)
{
	for (struct loom *loom = sys->looms; loom; loom = loom->next) {
		if (strcmp(loom->id, id) == 0)
			return loom;
	}
	return NULL;
}

static struct loom *
create_loom(struct system *sys, const char *relpath)
{
	char name[PATH_MAX];
	if (snprintf(name, PATH_MAX, "%s", relpath) >= PATH_MAX) {
		err("path too long: %s", relpath);
		return NULL;
	}

	if (strtok(name, "/") == NULL) {
		err("cannot find first '/': %s", relpath);
		return NULL;
	}

	struct loom *loom = find_loom(sys, name);

	if (loom == NULL) {
		loom = malloc(sizeof(struct loom));

		if (loom == NULL) {
			err("malloc failed:");
			return NULL;
		}

		if (loom_init_begin(loom, name) != 0) {
			err("loom_init_begin failed");
			return NULL;
		}

		DL_APPEND(sys->looms, loom);
		sys->nlooms++;
	}

	return loom;
}

static int
report_libovni_version(struct system *sys)
{
	int mixed = 0;
	const char *version = NULL;
	const char *commit = NULL;
	for (struct thread *th = sys->threads; th; th = th->gnext) {
		if (th->meta == NULL) {
			err("thread has no metadata %s", th->id);
			return -1;
		}

		const char *t_version = json_object_dotget_string(th->meta, "ovni.lib.version");

		if (t_version == NULL) {
			err("missing ovni.lib.version key in thread metadata: %s", th->id);
			return -1;
		}

		const char *t_commit = json_object_dotget_string(th->meta, "ovni.lib.commit");

		if (t_commit == NULL) {
			err("missing ovni.lib.commit key in thread metadata: %s", th->id);
			return -1;
		}

		if (version == NULL)
			version = t_version;

		if (commit == NULL)
			commit = t_commit;

		if (strcmp(t_version, version) != 0) {
			warn("thread is using a different libovni version (%s): %s",
					t_version, th->id);
			mixed = 1;
		}

		if (strcmp(t_commit, commit) != 0) {
			warn("thread is using a different libovni commit (%s): %s",
					t_commit, th->id);
			mixed = 1;
		}
	}

	if (mixed) {
		warn("mixed versions of libovni detected");
	} else {
		info("generated with libovni version %s commit %s", version, commit);
	}

	return 0;
}

static int
is_thread_stream(struct stream *s)
{
	JSON_Object *meta = stream_metadata(s);
	if (meta == NULL) {
		err("no metadata for stream: %s", s->relpath);
		return -1;
	}

	/* All streams must have a ovni.part attribute */
	const char *part_type = json_object_dotget_string(meta, "ovni.part");

	if (part_type == NULL) {
		err("cannot get attribute ovni.part for stream: %s",
				s->relpath);
		return -1;
	}

	if (strcmp(part_type, "thread") == 0) {
		return 1;
	}

	return 0;
}

static int
create_system(struct system *sys, struct trace *trace)
{
	const char *dir = trace->tracedir;

	/* Allocate the lpt map */
	sys->lpt = calloc((size_t) trace->nstreams, sizeof(struct lpt));
	if (sys->lpt == NULL) {
		err("calloc failed:");
		return -1;
	}

	size_t i = 0;
	for (struct stream *s = trace->streams; s ; s = s->next) {
		int m = is_thread_stream(s);
		if (m < 0) {
			err("is_thread_stream failed");
			return -1;
		} else if (m == 0) {
			warn("ignoring unknown stream %s", s->relpath);
			continue;
		}

		struct loom *loom = create_loom(sys, s->relpath);
		if (loom == NULL) {
			err("create_loom failed");
			return -1;
		}

		/* Loads metadata too */
		struct proc *proc = create_proc(loom, dir, s->relpath);
		if (proc == NULL) {
			err("create_proc failed");
			return -1;
		}

		struct thread *thread = create_thread(proc, dir, s->relpath);
		if (thread == NULL) {
			err("create_thread failed");
			return -1;
		}

		struct lpt *lpt = &sys->lpt[i++];
		lpt->stream = s;
		lpt->loom = loom;
		lpt->proc = proc;
		lpt->thread = thread;

		stream_data_set(s, lpt);
	}

	/* Ensure all looms have at least one CPU */
	for (struct loom *l = sys->looms; l; l = l->next) {
		if (l->ncpus == 0) {
			err("loom %s has no physical CPUs", l->id);
			return -1;
		}
	}

	return 0;
}

static int
cmp_loom_id(struct loom *a, struct loom *b)
{
	return strcmp(a->id, b->id);
}

static int
cmp_loom_rank(struct loom *a, struct loom *b)
{
	int id1 = a->rank_min;
	int id2 = b->rank_min;

	if (id1 < id2)
		return -1;
	if (id1 > id2)
		return +1;
	else
		return 0;
}

static void
sort_lpt(struct system *sys)
{
	if (sys->sort_by_rank) {
		info("sorting looms by rank");
		DL_SORT(sys->looms, cmp_loom_rank);
	} else {
		info("sorting looms by name");
		DL_SORT(sys->looms, cmp_loom_id);
	}

	for (struct loom *l = sys->looms; l; l = l->next)
		loom_sort(l);
}

static void
init_global_lists(struct system *sys)
{
	for (struct loom *l = sys->looms; l; l = l->next) {
		for (struct proc *p = l->procs; p; p = p->hh.next) {
			for (struct thread *t = p->threads; t; t = t->hh.next) {
				DL_APPEND2(sys->threads, t, gprev, gnext);
			}
			DL_APPEND2(sys->procs, p, gprev, gnext);
		}

		/* Looms are already in sys->looms */

		/* Add CPUs too */
		for (struct cpu *cpu = l->cpus; cpu; cpu = cpu->hh.next)
			DL_APPEND2(sys->cpus, cpu, prev, next);

		/* Virtual CPU last */
		DL_APPEND2(sys->cpus, &l->vcpu, prev, next);
	}
}

static void
print_system(struct system *sys)
{
	err("content of system: ");
	for (struct loom *l = sys->looms; l; l = l->next) {
		err("%s gindex=%"PRIi64, l->id, l->gindex);
		err("+ %zd processes: ", l->nprocs);
		for (struct proc *p = l->procs; p; p = p->hh.next) {
			err("| %s gindex=%"PRIi64" pid=%d",
					p->id, p->gindex, p->pid);
			err("| + %d threads: ", p->nthreads);
			for (struct thread *t = p->threads; t; t = t->hh.next) {
				err("| | %s tid=%d",
						t->id, t->tid);
			}
		}
		err("+ %zd phy cpus: ", l->ncpus);
		for (struct cpu *cpu = l->cpus; cpu; cpu = cpu->hh.next) {
			err("| %s gindex=%"PRIi64" phyid=%d",
					cpu->name,
					cpu->gindex,
					cpu->phyid);
		}

		err("+ 1 virtual cpu: ");
		struct cpu *cpu = &l->vcpu;
		err("| %s gindex=%"PRIi64" phyid=%d",
				cpu->name,
				cpu->gindex,
				cpu->phyid);
	}
}

static void
init_global_indices(struct system *sys)
{
	size_t iloom = 0;
	for (struct loom *l = sys->looms; l; l = l->next)
		loom_set_gindex(l, (int64_t) iloom++);

	sys->nprocs = 0;
	for (struct proc *p = sys->procs; p; p = p->gnext)
		proc_set_gindex(p, (int64_t) sys->nprocs++);

	sys->nthreads = 0;
	for (struct thread *t = sys->threads; t; t = t->gnext)
		thread_set_gindex(t, (int64_t) sys->nthreads++);

	sys->ncpus = 0;
	sys->nphycpus = 0;
	for (struct cpu *c = sys->cpus; c; c = c->next) {
		cpu_set_gindex(c, (int64_t) sys->ncpus++);
		if (!c->is_virtual)
			sys->nphycpus++;
	}
}

static int
init_end_system(struct system *sys)
{
	for (struct loom *l = sys->looms; l; l = l->next) {
		for (struct proc *p = l->procs; p; p = p->hh.next) {
			for (struct thread *t = p->threads; t; t = t->hh.next) {
				if (thread_init_end(t) != 0) {
					err("thread_init_end failed");
					return -1;
				}
			}
			if (proc_init_end(p) != 0) {
				err("proc_init_end failed");
				return -1;
			}
		}
		for (struct cpu *cpu = l->cpus; cpu; cpu = cpu->hh.next) {
			if (cpu_init_end(cpu) != 0) {
				err("cpu_init_end failed");
				return -1;
			}
		}
		if (cpu_init_end(&l->vcpu) != 0) {
			err("cpu_init_end failed");
			return -1;
		}
		if (loom_init_end(l) != 0) {
			err("loom_init_end failed");
			return -1;
		}
	}

	info("loaded %zd looms, %zd processes, %zd threads and %zd cpus",
			sys->nlooms, sys->nprocs, sys->nthreads, sys->nphycpus);

	return 0;
}

static int
load_clock_offsets(struct clkoff *clkoff, struct emu_args *args)
{
	const char *tracedir = args->tracedir;
	char def_file[PATH_MAX];
	char def_name[] = "clock-offsets.txt";

	clkoff_init(clkoff);

	if (snprintf(def_file, PATH_MAX, "%s/%s",
				tracedir, def_name) >= PATH_MAX) {
		err("path too long");
		return -1;
	}

	const char *offset_file = args->clock_offset_file;
	int is_optional = 0;

	/* Use the default path if not given */
	if (offset_file == NULL) {
		offset_file = def_file;
		is_optional = 1;
	}

	FILE *f = fopen(offset_file, "r");

	if (f == NULL) {
		if (is_optional && errno == ENOENT)
			return 0;

		err("fopen %s failed:", offset_file);
		return -1;
	}

	if (clkoff_load(clkoff, f) != 0) {
		err("clkoff_load failed");
		return -1;
	}

	info("loaded clock offset table from '%s' with %d entries",
			offset_file, clkoff_count(clkoff));

	fclose(f);
	return 0;
}

static int
parse_clkoff_entry(struct loom *looms, struct clkoff_entry *entry)
{
	size_t matches = 0;

	/* Use the median as the offset */
	int64_t offset = (int64_t) entry->median;
	const char *host = entry->name;

	struct loom *loom;
	DL_FOREACH(looms, loom) {
		/* Match the hostname exactly */
		if (strcmp(loom->hostname, host) != 0)
			continue;

		if (loom->clock_offset != 0) {
			err("loom %s already has a clock offset",
					loom->id);
			return -1;
		}

		loom->clock_offset = offset;
		matches++;
	}

	if (matches == 0) {
		err("cannot find loom with hostname '%s'", host);
		return -1;
	}

	return 0;
}

static int
init_offsets(struct system *sys, struct trace *trace)
{
	struct clkoff *table = &sys->clkoff;
	int n = clkoff_count(table);

	/* If we have more than one hostname and no offset table has been found,
	 * we won't be able to synchronize the clocks */
	if (n == 0 && sys->nlooms > 1) {
		warn("no clock offset file loaded with %zd looms",
				sys->nlooms);
	}

	for (int i = 0; i < n; i++) {
		struct clkoff_entry *entry = clkoff_get(table, i);
		if (parse_clkoff_entry(sys->looms, entry) != 0) {
			err("cannot parse clock offset entry %d", i);
			return -1;
		}
	}

	for (struct stream *s = trace->streams; s; s = s->next) {
		struct lpt *lpt = system_get_lpt(s);
		/* Not LPT stream */
		if (lpt == NULL)
			continue;

		int64_t offset = lpt->loom->clock_offset;
		if (stream_clkoff_set(s, offset) != 0) {
			err("cannot set clock offset");
			return -1;
		}
	}

	return 0;
}

static int
set_sort_criteria(struct system *sys)
{
	int some_have = 0;
	int all_have = 1;
	for (struct loom *l = sys->looms; l; l = l->next) {
		if (l->rank_enabled)
			some_have = 1;
		else
			all_have = 0;
	}

	/* Only sort by rank if all looms have the rank information */
	if (all_have)
		sys->sort_by_rank = 1;
	else if (some_have)
		warn("missing rank in some looms, cannot sort CPUs by rank");

	return 0;
}

int
system_init(struct system *sys, struct emu_args *args, struct trace *trace)
{
	memset(sys, 0, sizeof(struct system));
	sys->args = args;

	/* Parse the trace and create the looms, procs and threads */
	if (create_system(sys, trace) != 0) {
		err("create system failed");
		return -1;
	}

	if (set_sort_criteria(sys) != 0) {
		err("set_sort_criteria failed");
		return -1;
	}

	/* Ensure they are sorted so they are easier to read */
	sort_lpt(sys);

	/* Init global lists after sorting */
	init_global_lists(sys);

	/* Now that we have loaded all resources, populate the indices */
	init_global_indices(sys);

	if (init_end_system(sys) != 0) {
		err("init_end_system failed");
		return -1;
	}

	if (report_libovni_version(sys) != 0) {
		err("report_libovni_version failed");
		return -1;
	}

	/* Load the clock offsets table */
	if (load_clock_offsets(&sys->clkoff, args) != 0) {
		err("load_clock_offsets() failed");
		return -1;
	}

	/* Set the offsets of the looms and streams */
	if (init_offsets(sys, trace) != 0) {
		err("init_offsets() failed");
		return -1;
	}

	/* Finaly dump the system */
	if (0)
		print_system(sys);

	return 0;
}

struct lpt *
system_get_lpt(struct stream *stream)
{
	struct lpt *lpt = stream_data_get(stream);

	if (lpt == NULL)
		return NULL;

	if (lpt->stream != stream)
		die("inconsistent stream in lpt map");

	return lpt;
}

int
system_connect(struct system *sys, struct bay *bay, struct recorder *rec)
{
	/* Create Paraver traces */
	struct pvt *pvt_cpu = NULL;
	if ((pvt_cpu = recorder_add_pvt(rec, "cpu", (long) sys->ncpus)) == NULL) {
		err("recorder_add_pvt failed");
		return -1;
	}

	struct pvt *pvt_th = NULL;
	if ((pvt_th = recorder_add_pvt(rec, "thread", (long) sys->nthreads)) == NULL) {
		err("recorder_add_pvt failed");
		return -1;
	}

	for (struct thread *th = sys->threads; th; th = th->gnext) {
		if (thread_connect(th, bay, rec) != 0) {
			err("thread_connect failed");
			return -1;
		}

		struct prf *prf = pvt_get_prf(pvt_th);
		char name[128];
		int appid = th->proc->appid;
		int tid = th->tid;
		if (snprintf(name, 128, "TH %d.%d", appid, tid) >= 128) {
			err("label too long");
			return -1;
		}

		if (prf_add(prf, (long) th->gindex, name) != 0) {
			err("prf_add failed for thread '%s'", th->id);
			return -1;
		}
	}

	struct pcf *pcf_th = pvt_get_pcf(pvt_th);
	if (thread_create_pcf_types(pcf_th) != 0) {
		err("thread_create_pcf_types failed");
		return -1;
	}

	struct pcf_type *affinity_type = thread_get_affinity_pcf_type(pcf_th);

	for (struct cpu *cpu = sys->cpus; cpu; cpu = cpu->next) {
		if (cpu_connect(cpu, bay, rec) != 0) {
			err("cpu_connect failed");
			return -1;
		}

		struct prf *prf = pvt_get_prf(pvt_cpu);
		if (prf_add(prf, (long) cpu->gindex, cpu->name) != 0) {
			err("prf_add failed for cpu '%s'", cpu->name);
			return -1;
		}

		if (cpu_add_to_pcf_type(cpu, affinity_type) == NULL) {
			err("cpu_add_to_pcf_type failed for cpu '%s'", cpu->name);
			return -1;
		}
	}

	return 0;
}
