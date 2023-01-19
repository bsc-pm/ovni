/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu_system.h"
#include "utlist.h"
#include <errno.h>

static int
has_prefix(const char *path, const char *prefix)
{
	if (strncmp(path, prefix, strlen(prefix)) != 0)
		return 0;

	return 1;
}

static struct emu_thread *
new_thread(struct emu_proc *proc, const char *tracedir, const char *name, const char *relpath)
{
	struct emu_thread *thread = calloc(1, sizeof(struct emu_thread));

	if (thread == NULL)
		die("calloc failed\n");

	if (snprintf(thread->name, PATH_MAX, "%s", name) >= PATH_MAX)
		die("new_thread: name too long: %s\n", name);

	if (snprintf(thread->path, PATH_MAX, "%s/%s", tracedir, relpath) >= PATH_MAX)
		die("new_thread: path too long: %s/%s\n", tracedir, relpath);

	if (snprintf(thread->relpath, PATH_MAX, "%s", relpath) >= PATH_MAX)
		die("new_thread: relative path too long: %s\n", relpath);

	thread->proc = proc;

	err("new thread '%s'\n", thread->name);

	return thread;
}

static struct emu_thread *
find_thread(struct emu_proc *proc, const char *name)
{
	for (struct emu_thread *t = proc->threads; t; t = t->lnext) {
		if (strcmp(t->name, name) == 0)
			return t;
	}
	return NULL;
}

static int
create_thread(struct emu_proc *proc, const char *tracedir, const char *relpath)
{
	char name[PATH_MAX];
	if (snprintf(name, PATH_MAX, "%s", relpath) >= PATH_MAX) {
		err("create_thread: path too long: %s\n", relpath);
		return -1;
	}

	if (strtok(name, "/") == NULL) {
		err("missing first slash\n");
		return -1;
	}

	if (strtok(NULL, "/") == NULL) {
		err("missing second slash\n");
		return -1;
	}

	char *threadname = strtok(NULL, "/");
	if (threadname == NULL) {
		err("missing thread name\n");
		return -1;
	}

	if (!has_prefix(threadname, "thread")) {
		err("warning: ignoring unknown thread stream %s\n",
				relpath);
		return 0;
	}

	struct emu_thread *thread = find_thread(proc, threadname);

	if (thread == NULL) {
		thread = new_thread(proc, tracedir, threadname, relpath);
		DL_APPEND2(proc->threads, thread, lprev, lnext);
		proc->nthreads++;
	}

	return 0;
}

static struct emu_proc *
new_proc(struct emu_loom *loom, const char *tracedir, const char *name)
{
	struct emu_proc *proc = calloc(1, sizeof(struct emu_proc));

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

static struct emu_proc *
find_proc(struct emu_loom *loom, const char *name)
{
	for (struct emu_proc *proc = loom->procs; proc; proc = proc->lnext) {
		if (strcmp(proc->name, name) == 0)
			return proc;
	}
	return NULL;
}

static int
create_proc(struct emu_loom *loom, const char *tracedir, const char *relpath)
{
	char name[PATH_MAX];
	if (snprintf(name, PATH_MAX, "%s", relpath) >= PATH_MAX) {
		err("create_proc: path too long: %s\n", relpath);
		return -1;
	}

	if (strtok(name, "/") == NULL) {
		err("missing first slash\n");
		return -1;
	}

	char *procname = strtok(NULL, "/");
	if (procname == NULL) {
		err("missing proc name\n");
		return -1;
	}

	if (!has_prefix(procname, "proc")) {
		err("warning: ignoring unknown proc stream %s\n",
				relpath);
		return 0;
	}

	struct emu_proc *proc = find_proc(loom, procname);

	if (proc == NULL) {
		proc = new_proc(loom, tracedir, procname);
		DL_APPEND2(loom->procs, proc, lprev, lnext);
		loom->nprocs++;
	}

	return create_thread(proc, tracedir, relpath);
}

static struct emu_loom *
find_loom(struct emu_system *sys, const char *name)
{
	for (struct emu_loom *loom = sys->looms; loom; loom = loom->next) {
		if (strcmp(loom->name, name) == 0)
			return loom;
	}
	return NULL;
}

static struct emu_loom *
new_loom(const char *tracedir, const char *name)
{
	struct emu_loom *loom = calloc(1, sizeof(struct emu_loom));

	if (loom == NULL)
		die("calloc failed\n");

	if (snprintf(loom->name, PATH_MAX, "%s", name) >= PATH_MAX)
		die("new_loom: name too long: %s\n", name);

	if (snprintf(loom->relpath, PATH_MAX, "%s", name) >= PATH_MAX)
		die("new_loom: relative path too long: %s\n", name);

	if (snprintf(loom->path, PATH_MAX, "%s/%s", tracedir, loom->relpath) >= PATH_MAX)
		die("new_loom: path too long: %s/%s\n", tracedir, loom->relpath);

	err("new loom '%s'\n", loom->name);

	return loom;
}

static int
create_loom(struct emu_system *sys, const char *tracedir, const char *relpath)
{
	char name[PATH_MAX];
	if (snprintf(name, PATH_MAX, "%s", relpath) >= PATH_MAX) {
		err("create_loom: path too long: %s\n", relpath);
		return -1;
	}

	if (strtok(name, "/") == NULL) {
		err("create_looms: cannot find first '/': %s\n",
				relpath);
		return -1;
	}
	
	struct emu_loom *loom = find_loom(sys, name);

	if (loom == NULL) {
		loom = new_loom(tracedir, name);
		DL_APPEND(sys->looms, loom);
		sys->nlooms++;
	}

	return create_proc(loom, tracedir, relpath);
}

static int
create_system(struct emu_system *sys, struct emu_trace *trace)
{
	for (struct emu_stream *s = trace->streams; s ; s = s->next) {
		if (!has_prefix(s->relpath, "loom")) {
			err("warning: ignoring unknown steam %s\n",
					s->relpath);
			continue;
		}

		if (create_loom(sys, trace->tracedir, s->relpath) != 0) {
			err("create loom failed\n");
			return -1;
		}
	}

	return 0;
}

static int
cmp_thread(struct emu_thread *a, struct emu_thread *b)
{
	return strcmp(a->name, b->name);
}

static void
sort_proc(struct emu_proc *proc)
{
	DL_SORT2(proc->threads, cmp_thread, lprev, lnext);
}

static int
cmp_proc(struct emu_proc *a, struct emu_proc *b)
{
	return strcmp(a->name, b->name);
}

static void
sort_loom(struct emu_loom *loom)
{
	DL_SORT2(loom->procs, cmp_proc, lprev, lnext);

	for (struct emu_proc *p = loom->procs; p; p = p->gnext)
		sort_proc(p);
}

static int
cmp_loom(struct emu_loom *a, struct emu_loom *b)
{
	return strcmp(a->name, b->name);
}

static void
sort_system(struct emu_system *sys)
{
	DL_SORT(sys->looms, cmp_loom);

	for (struct emu_loom *l = sys->looms; l; l = l->next)
		sort_loom(l);
}

static void
init_global_lpt_lists(struct emu_system *sys)
{
	for (struct emu_loom *l = sys->looms; l; l = l->next) {
		for (struct emu_proc *p = l->procs; p; p = p->lnext) {
			for (struct emu_thread *t = p->threads; t; t = t->lnext) {
				DL_APPEND2(sys->threads, t, gprev, gnext);
			}
			DL_APPEND2(sys->procs, p, gprev, gnext);
		}

		/* Looms are already in sys->looms */
	}
}

static void
init_global_cpus_list(struct emu_system *sys)
{
	for (struct emu_loom *l = sys->looms; l; l = l->next) {
		for (size_t i = 0; i < l->ncpus; i++) {
			struct emu_cpu *cpu = &l->cpu[i];
			DL_APPEND2(sys->cpus, cpu, prev, next);
		}

		/* Virtual CPU last */
		DL_APPEND2(sys->cpus, &l->vcpu, prev, next);
	}
}

static void
print_system(struct emu_system *sys)
{
	err("content of system:\n");
	for (struct emu_loom *l = sys->looms; l; l = l->next) {
		err("%s\n", l->name);
		err("- gindex %ld\n", l->gindex);
		err("- path %s\n", l->path);
		err("- relpath %s\n", l->relpath);
		err("- processes:\n");
		for (struct emu_proc *p = l->procs; p; p = p->lnext) {
			err("  %s\n", p->name);
			err("  - gindex %ld\n", p->gindex);
			err("  - path %s\n", p->path);
			err("  - relpath %s\n", p->relpath);
			err("  - threads:\n");
			for (struct emu_thread *t = p->threads; t; t = t->lnext) {
				err("    %s\n", t->name);
				err("    - gindex %ld\n", t->gindex);
				err("    - path %s\n", t->path);
				err("    - relpath %s\n", t->relpath);
			}
		}
		err("- cpus:\n");
		for (size_t i = 0; i < l->ncpus; i++) {
			struct emu_cpu *cpu = &l->cpu[i];
			err("  %s\n", cpu->name);
			err("  - i %d\n", cpu->i);
			err("  - phyid %d\n", cpu->phyid);
			err("  - gindex %ld\n", cpu->gindex);
		}

		struct emu_cpu *cpu = &l->vcpu;
		err("- %s\n", cpu->name);
		err("  - i %d\n", cpu->i);
		err("  - phyid %d\n", cpu->phyid);
		err("  - gindex %ld\n", cpu->gindex);
	}
}

static int
load_proc_attributes(struct emu_proc *proc, const char *path)
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
load_proc_metadata(struct emu_proc *proc)
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
load_metadata(struct emu_system *sys)
{
	for (struct emu_proc *p = sys->procs; p; p = p->gnext) {
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
add_new_cpu(struct emu_loom *loom, int i, int phyid)
{
	struct emu_cpu *cpu = &loom->cpu[i];

	if (i < 0 || i >= (int) loom->ncpus) {
		err("add_new_cpu: new CPU i=%d out of bounds in %s\n",
				i, loom->relpath);
		return -1;
	}

	if (cpu->state != CPU_ST_UNKNOWN) {
		die("add_new_cpu: new CPU i=%d in unexpected in %s\n",
				i, loom->relpath);
		return -1;
	}

	cpu->state = CPU_ST_READY;
	cpu->i = i;
	cpu->phyid = phyid;
	cpu->loom = loom;
	cpu->is_virtual = 0;

	return 0;
}

static int
load_proc_cpus(struct emu_proc *proc)
{
	JSON_Object *meta = json_value_get_object(proc->meta);
	if (meta == NULL) {
		err("load_proc_cpus: json_value_get_object() failed\n");
		return -1;
	}

	JSON_Array *cpuarray = json_object_get_array(meta, "cpus");

	/* This process doesn't have the cpu list, but it should */
	if (cpuarray == NULL) {
		err("load_proc_cpus: json_object_get_array() failed\n");
		return -1;
	}

	size_t ncpus = json_array_get_count(cpuarray);
	if (ncpus == 0) {
		err("load_proc_cpus: the 'cpus' array is empty in metadata of %s\n",
				proc->relpath);
		return -1;
	}

	struct emu_loom *loom = proc->loom;
	loom->ncpus = ncpus;
	loom->cpu = calloc(ncpus, sizeof(struct emu_cpu));

	if (loom->cpu == NULL) {
		err("load_proc_cpus: calloc failed: %s\n", strerror(errno));
		return -1;
	}

	for (size_t i = 0; i < ncpus; i++) {
		JSON_Object *cpu = json_array_get_object(cpuarray, i);

		if (cpu == NULL) {
			err("proc_load_cpus: json_array_get_object() failed for cpu\n");
			return -1;
		}

		int index = (int) json_object_get_number(cpu, "index");
		int phyid = (int) json_object_get_number(cpu, "phyid");

		if (add_new_cpu(loom, index, phyid) != 0) {
			err("proc_load_cpus: add_new_cpu() failed\n");
			return -1;
		}
	}

	return 0;
}

static int
init_virtual_cpu(struct emu_loom *loom)
{
	struct emu_cpu *vcpu = &loom->vcpu;

	if (vcpu->state != CPU_ST_UNKNOWN) {
		err("init_virtual_cpu: unexpected virtual CPU state in %s\n",
				loom->relpath);
		return -1;
	}

	vcpu->state = CPU_ST_READY;
	vcpu->i = -1;
	vcpu->phyid = -1;
	vcpu->loom = loom;
	vcpu->is_virtual = 1;

	return 0;
}


static int
load_loom_cpus(struct emu_loom *loom)
{
	/* The process that contains the CPU list */
	struct emu_proc *proc_cpus = NULL;

	/* Search for the cpu list in all processes.
	 * Only one should contain it. */
	for (struct emu_proc *p = loom->procs; p; p = p->lnext) {
		if (!has_cpus_array(p->meta))
			continue;

		if (proc_cpus != NULL) {
			err("load_loom_cpus: duplicated cpu list provided in '%s' and '%s'\n",
					proc_cpus->relpath,
					p->relpath);
			return -1;
		}

		if (load_proc_cpus(p) != 0) {
			err("load_loom_cpus: load_proc_cpus failed: %s\n",
					p->relpath);
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

	return 0;
}

/* Obtain CPUs in the metadata files and other data */
static int
init_cpus(struct emu_system *sys)
{
	for (struct emu_loom *l = sys->looms; l; l = l->next) {
		if (load_loom_cpus(l) != 0) {
			err("init_cpus: load_loom_cpus() failed\n");
			return -1;
		}

		if (init_virtual_cpu(l) != 0) {
			err("init_cpus: init_virtual_cpu() failed\n");
			return -1;
		}
	}

	return 0;
}

static void
init_global_indices(struct emu_system *sys)
{
	size_t iloom = 0;
	for (struct emu_loom *l = sys->looms; l; l = l->next)
		l->gindex = iloom++;

	sys->nprocs = 0;
	for (struct emu_proc *p = sys->procs; p; p = p->gnext)
		p->gindex = sys->nprocs++;

	sys->nthreads = 0;
	for (struct emu_thread *t = sys->threads; t; t = t->gnext)
		t->gindex = sys->nprocs++;

	sys->ncpus = 0;
	for (struct emu_cpu *c = sys->cpus; c; c = c->next)
		c->gindex = sys->ncpus++;
}

static int
init_cpu_name(struct emu_cpu *cpu)
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
init_cpu_names(struct emu_system *sys)
{
	for (struct emu_cpu *cpu = sys->cpus; cpu; cpu = cpu->next) {
		if (init_cpu_name(cpu) != 0)
			return -1;
	}

	return 0;
}

int
emu_system_load(struct emu_system *sys, struct emu_trace *trace)
{
	/* Parse the emu_trace and create the looms, procs and threads */
	if (create_system(sys, trace) != 0) {
		err("emu_system_load: create system failed\n");
		return -1;
	}

	/* Ensure they are sorted so they are easier to read */
	sort_system(sys);

	/* Init global lists build after sorting */
	init_global_lpt_lists(sys);

	/* Now load all process metadata and set attributes */
	if (load_metadata(sys) != 0) {
		err("emu_system_load: load_metadata() failed\n");
		return -1;
	}

	/* From the metadata extract the CPUs too */
	if (init_cpus(sys) != 0) {
		err("emu_system_load: load_cpus() failed\n");
		return -1;
	}

	init_global_cpus_list(sys);

	/* Now that we have loaded all resources, populate the indices */
	init_global_indices(sys);

	if (init_cpu_names(sys) != 0) {
		err("emu_system_load: init_cpu_names() failed\n");
		return -1;
	}

	/* Finaly dump the system */
	print_system(sys);

	return 0;
}
