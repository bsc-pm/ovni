/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "system.h"
#include "utlist.h"
#include <errno.h>

static int
has_prefix(const char *path, const char *prefix)
{
	if (strncmp(path, prefix, strlen(prefix)) != 0)
		return 0;

	return 1;
}

static struct thread *
new_thread(struct proc *proc, const char *tracedir, const char *name, struct emu_stream *stream)
{
	struct thread *thread = calloc(1, sizeof(struct thread));

	if (thread == NULL)
		die("calloc failed\n");

	if (snprintf(thread->name, PATH_MAX, "%s", name) >= PATH_MAX)
		die("new_thread: name too long: %s\n", name);

	if (snprintf(thread->path, PATH_MAX, "%s/%s", tracedir, stream->relpath) >= PATH_MAX)
		die("new_thread: path too long: %s/%s\n", tracedir, stream->relpath);

	if (snprintf(thread->relpath, PATH_MAX, "%s", stream->relpath) >= PATH_MAX)
		die("new_thread: relative path too long: %s\n", stream->relpath);

	thread->proc = proc;
	thread->stream = stream;

	err("new thread '%s'\n", thread->name);

	return thread;
}

static struct thread *
find_thread(struct proc *proc, const char *name)
{
	for (struct thread *t = proc->threads; t; t = t->lnext) {
		if (strcmp(t->name, name) == 0)
			return t;
	}
	return NULL;
}

static struct thread *
create_thread(struct proc *proc, const char *tracedir, struct emu_stream *stream)
{
	char name[PATH_MAX];
	if (snprintf(name, PATH_MAX, "%s", stream->relpath) >= PATH_MAX) {
		err("create_thread: path too long: %s\n",
				stream->relpath);
		return NULL;
	}

	if (strtok(name, "/") == NULL) {
		err("missing first slash\n");
		return NULL;
	}

	if (strtok(NULL, "/") == NULL) {
		err("missing second slash\n");
		return NULL;
	}

	char *threadname = strtok(NULL, "/");
	if (threadname == NULL) {
		err("missing thread name\n");
		return NULL;
	}

	if (!has_prefix(threadname, "thread")) {
		err("unknown thread stream prefix %s\n",
				stream->relpath);
		return NULL;
	}

	struct thread *thread = find_thread(proc, threadname);

	if (thread != NULL) {
		err("create_thread: thread already exists: %s\n", threadname);
		return NULL;
	}

	thread = new_thread(proc, tracedir, threadname, stream);
	DL_APPEND2(proc->threads, thread, lprev, lnext);
	proc->nthreads++;

	return thread;
}

static struct proc *
new_proc(struct loom *loom, const char *tracedir, const char *name)
{
	struct proc *proc = calloc(1, sizeof(struct proc));

	if (proc == NULL)
		die("calloc failed\n");

	if (snprintf(proc->name, PATH_MAX, "%s", name) >= PATH_MAX)
		die("new_proc: name too long: %s\n", name);

	if (snprintf(proc->relpath, PATH_MAX, "%s/%s", loom->name, proc->name) >= PATH_MAX)
		die("new_proc: relative path too long: %s/%s", loom->name, proc->name);

	if (snprintf(proc->path, PATH_MAX, "%s/%s", tracedir, proc->relpath) >= PATH_MAX)
		die("new_proc: path too long: %s/%s", tracedir, proc->relpath);

	proc->loom = loom;

	err("new proc '%s'\n", proc->name);

	return proc;
}

static struct proc *
find_proc(struct loom *loom, const char *name)
{
	for (struct proc *proc = loom->procs; proc; proc = proc->lnext) {
		if (strcmp(proc->name, name) == 0)
			return proc;
	}
	return NULL;
}

static struct proc *
create_proc(struct loom *loom, const char *tracedir, const char *relpath)
{
	char name[PATH_MAX];
	if (snprintf(name, PATH_MAX, "%s", relpath) >= PATH_MAX) {
		err("create_proc: path too long: %s\n", relpath);
		return NULL;
	}

	if (strtok(name, "/") == NULL) {
		err("missing first slash\n");
		return NULL;
	}

	char *procname = strtok(NULL, "/");
	if (procname == NULL) {
		err("missing proc name\n");
		return NULL;
	}

	if (!has_prefix(procname, "proc")) {
		err("unknown proc stream prefix %s\n",
				relpath);
		return NULL;
	}

	struct proc *proc = find_proc(loom, procname);

	if (proc == NULL) {
		proc = new_proc(loom, tracedir, procname);
		DL_APPEND2(loom->procs, proc, lprev, lnext);
		loom->nprocs++;
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

static void
set_loom_hostname(char host[PATH_MAX], const char loom_name[PATH_MAX])
{
	/* Copy until dot or end */
	int i;
	for (i = 0; i < PATH_MAX - 1; i++) {
		if (loom_name[i] == '.' || loom_name[i] == '\0')
			break;

		host[i] = loom_name[i];
	}

	host[i] = '\0';
}

static struct loom *
new_loom(const char *tracedir, const char *name)
{
	struct loom *loom = calloc(1, sizeof(struct loom));

	if (loom == NULL)
		die("calloc failed\n");

	if (loom_init_begin(loom, name) != 0)
		re

	if (snprintf(loom->name, PATH_MAX, "%s", name) >= PATH_MAX)
		die("new_loom: name too long: %s\n", name);

	if (snprintf(loom->relpath, PATH_MAX, "%s", name) >= PATH_MAX)
		die("new_loom: relative path too long: %s\n", name);

	if (snprintf(loom->path, PATH_MAX, "%s/%s", tracedir, loom->relpath) >= PATH_MAX)
		die("new_loom: path too long: %s/%s\n", tracedir, loom->relpath);

	set_loom_hostname(loom->hostname, loom->name);

	err("new loom '%s'\n", loom->name);

	return loom;
}

static struct loom *
create_loom(struct system *sys, const char *tracedir, const char *relpath)
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
create_lpt(struct system *sys, struct emu_trace *trace)
{
	const char *dir = trace->tracedir;
	for (struct emu_stream *s = trace->streams; s ; s = s->next) {
		if (!loom_matches(s->relpath)) {
			err("warning: ignoring unknown stream %s", s->relpath);
			continue;
		}

		struct loom *loom = create_loom(sys, dir, s->relpath);
		if (loom == NULL) {
			err("create_loom failed");
			return -1;
		}

		struct proc *proc = create_proc(loom, dir, s->relpath);
		if (proc == NULL) {
			err("create_proc failed");
			return -1;
		}

		/* The thread sets the stream */
		struct thread *thread = create_thread(proc, dir, s);
		if (thread == NULL) {
			err("create_thread failed");
			return -1;
		}
	}

	return 0;
}

static int
cmp_thread(struct thread *a, struct thread *b)
{
	return strcmp(a->name, b->name);
}

static void
sort_proc(struct proc *proc)
{
	DL_SORT2(proc->threads, cmp_thread, lprev, lnext);
}

static int
cmp_proc(struct proc *a, struct proc *b)
{
	return strcmp(a->name, b->name);
}

static void
sort_loom(struct loom *loom)
{
	DL_SORT2(loom->procs, cmp_proc, lprev, lnext);

	for (struct proc *p = loom->procs; p; p = p->gnext)
		sort_proc(p);
}

static int
cmp_loom(struct loom *a, struct loom *b)
{
	return strcmp(a->name, b->name);
}

static void
sort_lpt(struct system *sys)
{
	DL_SORT(sys->looms, cmp_loom);

	for (struct loom *l = sys->looms; l; l = l->next)
		sort_loom(l);
}

static void
init_global_lpt_lists(struct system *sys)
{
	for (struct loom *l = sys->looms; l; l = l->next) {
		for (struct proc *p = l->procs; p; p = p->lnext) {
			for (struct thread *t = p->threads; t; t = t->lnext) {
				DL_APPEND2(sys->threads, t, gprev, gnext);
			}
			DL_APPEND2(sys->procs, p, gprev, gnext);
		}

		/* Looms are already in sys->looms */
	}
}

static void
init_global_cpus_list(struct system *sys)
{
	for (struct loom *l = sys->looms; l; l = l->next) {
		for (size_t i = 0; i < l->ncpus; i++) {
			struct cpu *cpu = &l->cpu[i];
			DL_APPEND2(sys->cpus, cpu, prev, next);
		}

		/* Virtual CPU last */
		DL_APPEND2(sys->cpus, &l->vcpu, prev, next);
	}
}

static void
print_system(struct system *sys)
{
	err("content of system:\n");
	for (struct loom *l = sys->looms; l; l = l->next) {
		err("%s\n", l->name);
		err("- gindex %ld\n", l->gindex);
		err("- path %s\n", l->path);
		err("- relpath %s\n", l->relpath);
		err("- processes:\n");
		for (struct proc *p = l->procs; p; p = p->lnext) {
			err("  %s\n", p->name);
			err("  - gindex %ld\n", p->gindex);
			err("  - path %s\n", p->path);
			err("  - relpath %s\n", p->relpath);
			err("  - threads:\n");
			for (struct thread *t = p->threads; t; t = t->lnext) {
				err("    %s\n", t->name);
				err("    - gindex %ld\n", t->gindex);
				err("    - path %s\n", t->path);
				err("    - relpath %s\n", t->relpath);
			}
		}
		err("- cpus:\n");
		for (size_t i = 0; i < l->ncpus; i++) {
			struct cpu *cpu = &l->cpu[i];
			err("  %s\n", cpu->name);
			err("  - i %d\n", cpu->i);
			err("  - phyid %d\n", cpu->phyid);
			err("  - gindex %ld\n", cpu->gindex);
		}

		struct cpu *cpu = &l->vcpu;
		err("- %s\n", cpu->name);
		err("  - i %d\n", cpu->i);
		err("  - phyid %d\n", cpu->phyid);
		err("  - gindex %ld\n", cpu->gindex);
	}
}

static int
load_proc_attributes(struct proc *proc, const char *path)
{
	JSON_Object *meta = json_value_get_object(proc->meta);
	if (meta == NULL) {
		err("load_proc_attributes: json_value_get_object() failed\n");
		return -1;
	}

	JSON_Value *appid_val = json_object_get_value(meta, "app_id");
	if (appid_val == NULL) {
		err("load_proc_attributes: missing app_id in metadata %s\n", path);
		return -1;
	}

	proc->appid = (int) json_number(appid_val);

	JSON_Value *rank_val = json_object_get_value(meta, "rank");

	if (rank_val != NULL) {
		proc->rank = (int) json_number(rank_val);
		proc->loom->rank_enabled = 1;
	} else {
		proc->rank = -1;
	}

	return 0;
}

static int
check_proc_metadata(JSON_Value *pmeta, const char *path)
{
	JSON_Object *meta = json_value_get_object(pmeta);
	if (meta == NULL) {
		err("check_proc_metadata: json_value_get_object() failed for %s\n",
				path);
		return -1;
	}

	JSON_Value *version_val = json_object_get_value(meta, "version");
	if (version_val == NULL) {
		err("check_proc_metadata: missing attribute \"version\" in %s\n",
				path);
		return -1;
	}

	int version = (int) json_number(version_val);

	if (version != OVNI_METADATA_VERSION) {
		err("check_proc_metadata: metadata version mismatch %d (expected %d) in %s\n",
				version, OVNI_METADATA_VERSION, path);
		return -1;
	}

	JSON_Value *mversion_val = json_object_get_value(meta, "model_version");
	if (mversion_val == NULL) {
		err("check_proc_metadata: missing attribute \"model_version\" in %s\n",
				path);
		return -1;
	}

	const char *mversion = json_string(mversion_val);
	if (strcmp(mversion, OVNI_MODEL_VERSION) != 0) {
		err("check_proc_metadata: model version mismatch '%s' (expected '%s') in %s\n",
				mversion, OVNI_MODEL_VERSION, path);
		return -1;
	}

	return 0;
}

static int
load_proc_metadata(struct proc *proc)
{
	char path[PATH_MAX];
	if (snprintf(path, PATH_MAX, "%s/%s", proc->path, "metadata.json") >= PATH_MAX) {
		err("load_proc_metadata: path too large: %s/%s\n",
				proc->path, "metadata.json");
		return -1;
	}

	proc->meta = json_parse_file_with_comments(path);
	if (proc->meta == NULL) {
		err("load_proc_metadata: json_parse_file_with_comments(%s) failed\n",
				path);
		return -1;
	}

	if (check_proc_metadata(proc->meta, path) != 0) {
		err("load_proc_metadata: invalid metadata\n");
		return -1;
	}

	/* The appid is populated from the metadata */
	if (load_proc_attributes(proc, path) != 0) {
		err("load_proc_metadata: invalid attributes\n");
		return -1;
	}

	return 0;
}

static int
load_metadata(struct system *sys)
{
	for (struct proc *p = sys->procs; p; p = p->gnext) {
		if (load_proc_metadata(p) != 0) {
			err("error loading metadata for %s\n", p->relpath);
			return -1;
		}
	}

	return 0;
}

static int
has_cpus_array(JSON_Value *metadata)
{
	JSON_Object *meta = json_value_get_object(metadata);
	if (meta == NULL) {
		err("has_cpus: json_value_get_object() failed\n");
		return -1;
	}

	/* Only check for the "cpus" key, if it has zero elements is an error
	 * that will be reported later */
	JSON_Array *cpuarray = json_object_get_array(meta, "cpus");

	if (cpuarray != NULL)
		return 1;

	return 0;
}

static int
load_proc_cpus(struct proc *proc)
{
	JSON_Object *meta = json_value_get_object(proc->meta);
	if (meta == NULL) {
		err("json_value_get_object() failed");
		return -1;
	}

	JSON_Array *cpuarray = json_object_get_array(meta, "cpus");

	/* This process doesn't have the cpu list, but it should */
	if (cpuarray == NULL) {
		err("json_object_get_array() failed");
		return -1;
	}

	size_t ncpus = json_array_get_count(cpuarray);
	if (ncpus == 0) {
		err("empty cpus array in metadata of %s", proc->id);
		return -1;
	}

	struct loom *loom = proc->loom;
	for (size_t i = 0; i < ncpus; i++) {
		JSON_Object *jcpu = json_array_get_object(cpuarray, i);
		if (jcpu == NULL) {
			err("json_array_get_object() failed for cpu");
			return -1;
		}

		/* Cast from double */
		int index = (int) json_object_get_number(jcpu, "index");
		int phyid = (int) json_object_get_number(jcpu, "phyid");

		struct cpu *cpu = calloc(1, sizeof(struct cpu));
		if (cpu == NULL) {
			err("calloc failed:");
			return -1;
		}

		cpu_init(cpu, i, phyid, 0);
		if (loom_add_cpu(loom, cpu) != 0) {
			err("loom_add_cpu() failed");
			return -1;
		}
	}

	return 0;
}

static int
load_loom_cpus(struct loom *loom)
{
	/* The process that contains the CPU list */
	struct proc *proc_cpus = NULL;

	/* Search for the cpu list in all processes.
	 * Only one should contain it. */
	for (struct proc *p = loom->procs; p; p = p->lnext) {
		if (!has_cpus_array(p->meta))
			continue;

		if (proc_cpus != NULL) {
			err("duplicated cpu list provided in '%s' and '%s'",
					proc_cpus->relpath,
					p->relpath);
			return -1;
		}

		if (load_proc_cpus(p) != 0) {
			err("load_proc_cpus failed: %s", p->id);
			return -1;
		}

		proc_cpus = p;
	}

	/* One of the process must have the list of CPUs */
	if (proc_cpus == NULL) {
		err("load_loom_cpus: no process contains a CPU list for %s\n",
				loom->relpath);
		return -1;
	}

	if (loom->ncpus == 0) {
		err("load_loom_cpus: no CPUs found in loom %s\n",
				loom->relpath);
		return -1;
	}

	/* Init virtual CPU too */
	cpu_init(&l->vcpu, l, -1, -1, 1);

	return 0;
}

/* Obtain CPUs in the metadata files and other data */
static int
init_cpus(struct system *sys)
{
	for (struct loom *l = sys->looms; l; l = l->next) {
		if (load_loom_cpus(l) != 0) {
			err("init_cpus: load_loom_cpus() failed\n");
			return -1;
		}
	}

	return 0;
}

static void
init_global_indices(struct system *sys)
{
	size_t iloom = 0;
	for (struct loom *l = sys->looms; l; l = l->next)
		loom_set_gindex(l, iloom++);

	sys->nprocs = 0;
	for (struct proc *p = sys->procs; p; p = p->gnext)
		p->gindex = sys->nprocs++;

	sys->nthreads = 0;
	for (struct thread *t = sys->threads; t; t = t->gnext)
		t->gindex = sys->nprocs++;

	sys->ncpus = 0;
	for (struct cpu *c = sys->cpus; c; c = c->next)
		cpu_set_gindex(c, sys->ncpus++);
}

static int
init_cpu_name(struct cpu *cpu)
{
	size_t i = cpu->loom->gindex;
	size_t j = cpu->i;
	int n = 0;

	if (cpu->is_virtual)
		n = snprintf(cpu->name, PATH_MAX, "vCPU %ld.*", i);
	else
		n = snprintf(cpu->name, PATH_MAX, "CPU %ld.%ld", i, j);

	if (n >= PATH_MAX) {
		err("init_cpu_name: cpu name too long\n");
		return -1;
	}

	return 0;
}

static int
init_cpu_names(struct system *sys)
{
	for (struct cpu *cpu = sys->cpus; cpu; cpu = cpu->next) {
		if (init_cpu_name(cpu) != 0)
			return -1;
	}

	return 0;
}

static void
link_streams_to_threads(struct system *sys)
{
	for (struct thread *th = sys->threads; th; th = th->gnext)
		emu_stream_data_set(th->stream, th);
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
		err("load_clock_offsets: path too long\n");
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
		if (is_optional) {
			return 0;
		}

		err("load_clock_offsets: fopen %s failed: %s\n",
				offset_file, strerror(errno));
		return -1;
	}

	if (clkoff_load(clkoff, f) != 0) {
		err("load_clock_offsets: clkoff_load failed\n");
	}

	err("loaded clock offset table from '%s' with %d entries\n",
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
			err("parse_clkoff_entry: loom %s already has a clock offset\n",
					loom->name);
			return -1;
		}

		loom->clock_offset = offset;
		matches++;
	}

	if (matches == 0) {
		err("parse_clkoff_entry: cannot find any loom with hostname '%s'\n",
				host);
		return -1;
	}

	return 0;
}

static int
init_offsets(struct system *sys)
{
	struct clkoff *table = &sys->clkoff;
	int n = clkoff_count(table);

	/* If we have more than one hostname and no offset table has been found,
	 * we won't be able to synchronize the clocks */
	if (n == 0 && sys->nlooms > 1) {
		err("warning: no clock offset file loaded with %ld looms\n",
				sys->nlooms);

		if (sys->args->linter_mode)
			abort();
	}

	for (int i = 0; i < n; i++) {
		struct clkoff_entry *entry = clkoff_get(table, i);
		if (parse_clkoff_entry(sys->looms, entry) != 0) {
			err("init_offsets: cannot parse clock offset entry %d\n", i);
			return -1;
		}
	}

	/* Set the stream clock offsets too */
	struct thread *thread;
	DL_FOREACH2(sys->threads, thread, gnext) {
		struct loom *loom = thread->proc->loom;
		int64_t offset = loom->clock_offset;
		if (emu_stream_clkoff_set(thread->stream, offset) != 0) {
			err("init_offsets: cannot set clock offset\n");
			return -1;
		}
	}

	return 0;
}

int
system_init(struct system *sys, struct emu_args *args, struct emu_trace *trace)
{
	memset(sys, 0, sizeof(struct system));
	sys->args = args;

	/* Parse the trace and create the looms, procs and threads */
	if (create_lpt(sys, trace) != 0) {
		err("system_init: create system failed\n");
		return -1;
	}


	/* Create global lists and indices */


	/* Setup names */


//	/* Ensure they are sorted so they are easier to read */
//	sort_lpt(sys);
//
//	/* Init global lists after sorting */
//	init_global_lpt_lists(sys);
//
//	/* Now load all process metadata and set attributes */
//	if (load_metadata(sys) != 0) {
//		err("system_init: load_metadata() failed\n");
//		return -1;
//	}
//
//	/* From the metadata extract the CPUs too */
//	if (init_cpus(sys) != 0) {
//		err("system_init: load_cpus() failed\n");
//		return -1;
//	}
//
//	init_global_cpus_list(sys);
//
//	/* Now that we have loaded all resources, populate the indices */
//	init_global_indices(sys);
//
//	/* Set CPU names like "CPU 1.34" */
//	if (init_cpu_names(sys) != 0) {
//		err("system_init: init_cpu_names() failed\n");
//		return -1;
//	}
//
//	/* We need to retrieve the thread from the stream too */
//	link_streams_to_threads(sys);
//
//	/* Load the clock offsets table */
//	if (load_clock_offsets(&sys->clkoff, args) != 0) {
//		err("system_init: load_clock_offsets() failed\n");
//		return -1;
//	}
//
//	/* Set the offsets of the looms and streams */
//	if (init_offsets(sys) != 0) {
//		err("system_init: init_offsets() failed\n");
//		return -1;
//	}
//
//	/* Finaly dump the system */
//	print_system(sys);

	return 0;
}
