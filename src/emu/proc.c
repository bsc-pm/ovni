/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "proc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "path.h"
#include "stream.h"
#include "thread.h"

int
proc_stream_get_pid(struct stream *s)
{
	JSON_Object *meta = stream_metadata(s);

	double pid = json_object_dotget_number(meta, "ovni.pid");

	/* Zero is used for errors, so forbidden for pid too */
	if (pid == 0) {
		err("cannot get attribute ovni.pid for stream: %s",
				s->relpath);
		return -1;
	}

	return (int) pid;
}

int
proc_init_begin(struct proc *proc, int pid)
{
	memset(proc, 0, sizeof(struct proc));

	proc->gindex = -1;
	proc->pid = pid;

	if (snprintf(proc->id, PATH_MAX, "proc.%d", pid) >= PATH_MAX) {
		err("path too long");
		return -1;
	}

	dbg("created proc %s", proc->id);

	return 0;
}

void
proc_set_gindex(struct proc *proc, int64_t gindex)
{
	proc->gindex = gindex;
}

void
proc_set_loom(struct proc *proc, struct loom *loom)
{
	proc->loom = loom;
}

int
proc_load_metadata(struct proc *proc, JSON_Object *meta)
{
	if (proc->metadata_loaded) {
		err("process %s already loaded metadata", proc->id);
		return -1;
	}

	JSON_Value *version_val = json_object_get_value(meta, "version");
	if (version_val == NULL) {
		err("missing attribute 'version' in metadata");
		return -1;
	}

	proc->metadata_version = (int) json_number(version_val);

	JSON_Value *appid_val = json_object_dotget_value(meta, "ovni.app_id");
	if (appid_val == NULL) {
		err("missing attribute 'ovni.app_id' in metadata");
		return -1;
	}

	proc->appid = (int) json_number(appid_val);

	JSON_Value *rank_val = json_object_dotget_value(meta, "ovni.rank");

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

	thread_set_proc(thread, proc);

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
