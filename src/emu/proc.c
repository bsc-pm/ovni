/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "proc.h"

#include "utlist.h"
#include "path.h"
#include <errno.h>

static int
get_pid(const char *id, int *pid)
{
	/* TODO: Store the PID the metadata.json instead */

	/* The id must be like "loom.host01.123/proc.345" */
	if (path_count(id, '/') != 1) {
		err("proc id can only contain one '/': %s", id);
		return -1;
	}

	/* Get the proc.345 part */
	const char *procname;
	if (path_next(id, '/', &procname) != 0) {
		err("cannot get proc name");
		return -1;
	}

	/* Ensure the prefix is ok */
	const char prefix[] = "proc.";
	if (!path_has_prefix(procname, prefix)) {
		err("proc name must start with '%s': %s", prefix, id);
		return -1;
	}

	/* Get the 345 part */
	const char *pidstr;
	if (path_next(procname, '.', &pidstr) != 0) {
		err("cannot find proc dot in '%s'", id);
		return -1;
	}

	*pid = atoi(pidstr);

	return 0;
}

int
proc_relpath_get_pid(const char *relpath, int *pid)
{
	char id[PATH_MAX];

	if (snprintf(id, PATH_MAX, "%s", relpath) >= PATH_MAX) {
		err("path too long");
		return -1;
	}

	if (path_keep(id, 2) != 0) {
		err("cannot delimite proc dir");
		return -1;
	}

	return get_pid(id, pid);
}

int
proc_init_begin(struct proc *proc, const char *relpath)
{
	memset(proc, 0, sizeof(struct proc));

	proc->gindex = -1;

	if (snprintf(proc->id, PATH_MAX, "%s", relpath) >= PATH_MAX) {
		err("path too long");
		return -1;
	}

	if (path_keep(proc->id, 2) != 0) {
		err("cannot delimite proc dir");
		return -1;
	}

	if (get_pid(proc->id, &proc->pid) != 0) {
		err("cannot parse proc pid");
		return -1;
	}

	err("created proc %s", proc->id);

	return 0;
}

void
proc_set_gindex(struct proc *proc, int64_t gindex)
{
	proc->gindex = gindex;
}

int
proc_load_metadata(struct proc *proc, JSON_Object *meta)
{
	if (proc->metadata_loaded) {
		err("process %s already loaded metadata", proc->id);
		return -1;
	}

	JSON_Value *appid_val = json_object_get_value(meta, "app_id");
	if (appid_val == NULL) {
		err("missing attribute 'app_id' in metadata\n");
		return -1;
	}

	proc->appid = (int) json_number(appid_val);

	JSON_Value *rank_val = json_object_get_value(meta, "rank");

	if (rank_val != NULL)
		proc->rank = (int) json_number(rank_val);
	else
		proc->rank = -1;

	proc->metadata_loaded = 1;

	return 0;
}

struct thread *
proc_find_thread(struct proc *proc, int tid)
{
	struct thread *thread = NULL;
	HASH_FIND_INT(proc->threads, &tid, thread);
	return thread;
}

int
proc_add_thread(struct proc *proc, struct thread *thread)
{
	if (proc->is_init) {
		err("cannot modify threads of initialized proc");
		return -1;
	}

	int tid = thread_get_tid(thread);

	if (proc_find_thread(proc, tid) != NULL) {
		err("thread with tid %d already in proc %s", tid, proc->id);
		return -1;
	}

	HASH_ADD_INT(proc->threads, tid, thread);
	proc->nthreads++;

	return 0;
}

static int
by_tid(struct thread *t1, struct thread *t2)
{
	int id1 = thread_get_tid(t1);
	int id2 = thread_get_tid(t2);

	if (id1 < id2)
		return -1;
	if (id1 > id2)
		return +1;
	else
		return 0;
}

void
proc_sort(struct proc *proc)
{
	HASH_SORT(proc->threads, by_tid);
}

int
proc_init_end(struct proc *proc)
{
	if (proc->gindex < 0) {
		err("gindex not set");
		return -1;
	}

	if (!proc->metadata_loaded) {
		err("metadata not loaded");
		return -1;
	}

	proc->is_init = 1;
	return 0;
}

int
proc_get_pid(struct proc *proc)
{
	return proc->pid;
}
