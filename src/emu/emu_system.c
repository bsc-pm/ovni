/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu_system.h"
#include "utlist.h"

static int
has_prefix(const char *path, const char *prefix)
{
	if (strncmp(path, prefix, strlen(prefix)) != 0)
		return 0;

	return 1;
}

static struct emu_thread *
new_thread(struct emu_proc *proc, const char *name, const char *relpath)
{
	struct emu_thread *thread = calloc(1, sizeof(struct emu_thread));

	if (thread == NULL)
		die("calloc failed\n");

	if (snprintf(thread->name, PATH_MAX, "%s", name) >= PATH_MAX)
		die("new_thread: name too long: %s\n", name);

	if (snprintf(thread->relpath, PATH_MAX, "%s", relpath) >= PATH_MAX)
		die("new_proc: relative path too long: %s\n", relpath);

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
create_thread(struct emu_proc *proc, const char *relpath)
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
		thread = new_thread(proc, threadname, relpath);
		DL_APPEND2(proc->threads, thread, lprev, lnext);
		proc->nthreads++;
	}

	return 0;
}

static struct emu_proc *
new_proc(struct emu_loom *loom, const char *name, const char *relpath)
{
	struct emu_proc *proc = calloc(1, sizeof(struct emu_proc));

	if (proc == NULL)
		die("calloc failed\n");

	if (snprintf(proc->name, PATH_MAX, "%s", name) >= PATH_MAX)
		die("new_proc: name too long: %s\n", name);

	if (snprintf(proc->relpath, PATH_MAX, "%s", relpath) >= PATH_MAX)
		die("new_proc: relative path too long: %s\n", relpath);

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
create_proc(struct emu_loom *loom, const char *relpath)
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
		proc = new_proc(loom, procname, relpath);
		DL_APPEND2(loom->procs, proc, lprev, lnext);
		loom->nprocs++;
	}

	return create_thread(proc, relpath);
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
new_loom(const char *name, const char *relpath)
{
	struct emu_loom *loom = calloc(1, sizeof(struct emu_loom));

	if (loom == NULL)
		die("calloc failed\n");

	if (snprintf(loom->name, PATH_MAX, "%s", name) >= PATH_MAX)
		die("new_loom: name too long: %s\n", name);

	if (snprintf(loom->relpath, PATH_MAX, "%s", relpath) >= PATH_MAX)
		die("new_loom: relative path too long: %s\n", relpath);

	err("new loom '%s'\n", loom->name);

	return loom;
}

static int
create_loom(struct emu_system *sys, const char *relpath)
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
		loom = new_loom(name, relpath);
		DL_APPEND(sys->looms, loom);
		sys->nlooms++;
	}

	return create_proc(loom, relpath);
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

		if (create_loom(sys, s->relpath) != 0) {
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
populate_global_lists(struct emu_system *sys)
{
	for (struct emu_loom *l = sys->looms; l; l = l->next) {
		for (struct emu_proc *p = l->procs; p; p = p->lnext) {
			for (struct emu_thread *t = p->threads; t; t = t->lnext) {
				DL_APPEND2(sys->threads, t, gprev, gnext);
			}
			DL_APPEND2(sys->procs, p, gprev, gnext);
		}
		/* Looms are already in emu_system */
	}
}

static void
print_system(struct emu_system *sys)
{
	err("content of system:\n");
	for (struct emu_loom *l = sys->looms; l; l = l->next) {
		err("%s\n", l->name);
		for (struct emu_proc *p = l->procs; p; p = p->lnext) {
			err("  %s\n", p->name);
			for (struct emu_thread *t = p->threads; t; t = t->lnext) {
				err("    %s\n", t->name);
			}
		}
	}
}

int
emu_system_load(struct emu_system *sys, struct emu_trace *trace)
{
	if (create_system(sys, trace) != 0) {
		err("emu_system_load: create system failed\n");
		return -1;
	}

	sort_system(sys);
	populate_global_lists(sys);
	print_system(sys);

	return 0;
}
