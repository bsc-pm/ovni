/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "system.h"
#include "utlist.h"
#include "metadata.h"
#include <errno.h>

static struct thread *
create_thread(struct proc *proc, const char *relpath)
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

	if (thread_init_begin(thread, proc, relpath) != 0) {
		err("cannot init thread");
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
		err("calloc failed:");
		return NULL;
	}

	if (proc_init_begin(proc, relpath) != 0) {
		err("proc_init_begin failed");
		return NULL;
	}

	if (loom_add_proc(loom, proc) != 0) {
		err("loom_add_proc failed");
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
	if (metadata_load(mpath, loom, proc) != 0) {
		err("cannot load metadata from %s", mpath);
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
			err("calloc failed:");
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
create_system(struct system *sys, struct trace *trace)
{
	const char *dir = trace->tracedir;

	/* Allocate the lpt map */
	sys->lpt = calloc(trace->nstreams, sizeof(struct lpt));
	if (sys->lpt == NULL) {
		err("calloc failed:");
		return -1;
	}

	size_t i = 0;
	for (struct stream *s = trace->streams; s ; s = s->next) {
		if (!loom_matches(s->relpath)) {
			err("warning: ignoring unknown stream %s", s->relpath);
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

		struct thread *thread = create_thread(proc, s->relpath);
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

	return 0;
}

static int
cmp_loom(struct loom *a, struct loom *b)
{
	return strcmp(a->id, b->id);
}

static void
sort_lpt(struct system *sys)
{
	DL_SORT(sys->looms, cmp_loom);

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
	err("content of system:\n");
	for (struct loom *l = sys->looms; l; l = l->next) {
		err("%s gindex=%d\n", l->id, l->gindex);
		err("+ %ld processes:\n", l->nprocs);
		for (struct proc *p = l->procs; p; p = p->hh.next) {
			err("| %s gindex=%d pid=%d\n",
					p->id, p->gindex, p->pid);
			err("| + %ld threads:\n", p->nthreads);
			for (struct thread *t = p->threads; t; t = t->hh.next) {
				err("| | %s tid=%d\n",
						t->id, t->tid);
			}
		}
		err("+ %ld phy cpus:\n", l->ncpus);
		for (struct cpu *cpu = l->cpus; cpu; cpu = cpu->hh.next) {
			err("| %s gindex=%ld phyid=%d\n",
					cpu->name,
					cpu->gindex,
					cpu->phyid);
			//err("  - i %d\n", cpu->i);
		}

		err("+ 1 virtual cpu:\n", l->ncpus);
		struct cpu *cpu = &l->vcpu;
		err("| %s gindex=%ld phyid=%d\n",
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
		loom_set_gindex(l, iloom++);

	sys->nprocs = 0;
	for (struct proc *p = sys->procs; p; p = p->gnext)
		proc_set_gindex(p, sys->nprocs++);

	sys->nthreads = 0;
	for (struct thread *t = sys->threads; t; t = t->gnext)
		thread_set_gindex(t, sys->nthreads++);

	sys->ncpus = 0;
	for (struct cpu *c = sys->cpus; c; c = c->next)
		cpu_set_gindex(c, sys->ncpus++);
}

static int
init_cpu_name(struct loom *loom, struct cpu *cpu, int virtual)
{
	size_t i = loom_get_gindex(loom);
	size_t j = cpu_get_phyid(cpu);
	int n = 0;
	char name[PATH_MAX];

	if (virtual)
		n = snprintf(name, PATH_MAX, "vCPU %ld.*", i);
	else
		n = snprintf(name, PATH_MAX, "CPU %ld.%ld", i, j);

	if (n >= PATH_MAX) {
		err("cpu name too long");
		return -1;
	}

	cpu_set_name(cpu, name);

	return 0;
}

static int
init_cpu_names(struct system *sys)
{
	for (struct loom *loom = sys->looms; loom; loom = loom->next) {
		for (struct cpu *cpu = loom->cpus; cpu; cpu = cpu->hh.next) {
			if (init_cpu_name(loom, cpu, 0) != 0)
				return -1;
		}
		struct cpu *vcpu = loom_get_vcpu(loom);
		if (init_cpu_name(loom, vcpu, 1) != 0)
			return -1;
	}

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
		if (is_optional)
			return 0;

		err("fopen %s failed:", offset_file);
		return -1;
	}

	if (clkoff_load(clkoff, f) != 0) {
		err("clkoff_load failed");
		return -1;
	}

	err("loaded clock offset table from '%s' with %d entries",
			offset_file, clkoff_count(clkoff));

	fclose(f);
	return 0;
}

static int
parse_clkoff_entry(struct loom *looms, struct clkoff_entry *entry)
{
	size_t matches = 0;

	/* Use the median as the offset */
	size_t offset = entry->median;
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
		err("warning: no clock offset file loaded with %ld looms",
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
		if (lpt == NULL) {
			err("cannot get stream lpt");
			return -1;
		}

		int64_t offset = lpt->loom->clock_offset;
		if (stream_clkoff_set(s, offset) != 0) {
			err("cannot set clock offset");
			return -1;
		}
	}

	return 0;
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

	/* Ensure they are sorted so they are easier to read */
	sort_lpt(sys);

	/* Init global lists after sorting */
	init_global_lists(sys);

	/* Now that we have loaded all resources, populate the indices */
	init_global_indices(sys);

	/* Set CPU names like "CPU 1.34" */
	if (init_cpu_names(sys) != 0) {
		err("init_cpu_names() failed");
		return -1;
	}

	/* Load the clock offsets table */
	if (load_clock_offsets(&sys->clkoff, args) != 0) {
		err("load_clock_offsets() failed");
		return -1;
	}

	/* Set the offsets of the looms and streams */
	if (init_offsets(sys, trace) != 0) {
		err("system_init: init_offsets() failed\n");
		return -1;
	}

	if (init_end_system(sys) != 0) {
		err("init_end_system failed");
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

	if (lpt->stream != stream)
		die("inconsistent stream in lpt map");

	return lpt;
}

int
system_connect(struct system *sys, struct bay *bay, struct recorder *rec)
{
	/* Create Paraver traces */
	struct pvt *pvt_cpu = NULL;
	if ((pvt_cpu = recorder_add_pvt(rec, "cpu", sys->ncpus)) == NULL) {
		err("recorder_add_pvt failed");
		return -1;
	}

	struct pvt *pvt_th = NULL;
	if ((pvt_th = recorder_add_pvt(rec, "thread", sys->nthreads)) == NULL) {
		err("recorder_add_pvt failed");
		return -1;
	}

	for (struct thread *th = sys->threads; th; th = th->gnext) {
		if (thread_connect(th, bay, rec) != 0) {
			err("thread_connect failed\n");
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

		if (prf_add(prf, th->gindex, name) != 0) {
			err("prf_add failed for thread '%s'", th->id);
			return -1;
		}
	}

	for (struct cpu *cpu = sys->cpus; cpu; cpu = cpu->next) {
		if (cpu_connect(cpu, bay, rec) != 0) {
			err("cpu_connect failed\n");
			return -1;
		}

		struct prf *prf = pvt_get_prf(pvt_cpu);
		if (prf_add(prf, cpu->gindex, cpu->name) != 0) {
			err("prf_add failed for cpu '%s'", cpu->name);
			return -1;
		}
	}

	return 0;
}
