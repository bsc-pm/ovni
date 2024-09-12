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
	proc->appid = 0;
	proc->rank = -1;
	proc->nranks = 0;
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

static int
load_appid(struct proc *proc, struct stream *s)
{
	JSON_Object *meta = stream_metadata(s);
	JSON_Value *appid_val = json_object_dotget_value(meta, "ovni.app_id");

	/* May not be present in all thread streams */
	if (appid_val == NULL)
		return 0;

	int appid = (int) json_number(appid_val);
	if (proc->appid && proc->appid != appid) {
		err("mismatch previous appid %d with stream: %s",
				proc->appid, s->relpath);
		return -1;
	}

	if (appid <= 0) {
		err("appid must be >0, stream: %s", s->relpath);
		return -1;
	}

	proc->appid = appid;
	return 0;
}

static int
load_rank(struct proc *proc, struct stream *s)
{
	JSON_Object *meta = stream_metadata(s);
	JSON_Value *rank_val = json_object_dotget_value(meta, "ovni.rank");

	/* Optional */
	if (rank_val == NULL) {
		dbg("process %s has no rank", proc->id);
		return 0;
	}

	int rank = (int) json_number(rank_val);

	if (rank < 0) {
		err("rank %d must be >=0, stream: %s", rank, s->relpath);
		return -1;
	}

	if (proc->rank >= 0 && proc->rank != rank) {
		err("mismatch previous rank %d with stream: %s",
				proc->rank, s->relpath);
		return -1;
	}

	/* Same with nranks, but it is not optional now */
	JSON_Value *nranks_val = json_object_dotget_value(meta, "ovni.nranks");
	if (nranks_val == NULL) {
		err("missing ovni.nranks attribute: %s", s->relpath);
		return -1;
	}

	int nranks = (int) json_number(nranks_val);

	if (nranks <= 0) {
		err("nranks %d must be >0, stream: %s", nranks, s->relpath);
		return -1;
	}

	if (proc->nranks > 0 && proc->nranks != nranks) {
		err("mismatch previous nranks %d with stream: %s",
				proc->nranks, s->relpath);
		return -1;
	}

	/* Ensure rank fits in nranks */
	if (rank >= nranks) {
		err("rank %d must be lower than nranks %d: %s",
				rank, nranks, s->relpath);
		return -1;
	}

	dbg("process %s rank=%d nranks=%d",
			proc->id, rank, nranks);
	proc->rank = rank;
	proc->nranks = nranks;

	return 0;
}

/** Merges the metadata from the stream in the process. */
int
proc_load_metadata(struct proc *proc, struct stream *s)
{
	if (load_appid(proc, s) != 0) {
		err("load_appid failed for stream: %s", s->relpath);
		return -1;
	}

	if (load_rank(proc, s) != 0) {
		err("load_rank failed for stream: %s", s->relpath);
		return -1;
	}

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

	if (proc->appid <= 0) {
		err("appid not set");
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
