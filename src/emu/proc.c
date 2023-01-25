/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "proc.h"

#include "utlist.h"
#include "path.h"
#include <errno.h>

static const char prefix[] = "proc.";

static int
set_id(struct proc *proc, const char *id)
{
	/* The id must be like "loom.123/proc.345" */

	const char *p = strchr(id, '/');
	if (p == NULL) {
		err("proc relpath missing '/': %s", id);
		return -1;
	}

	p++; /* Skip slash */
	if (strchr(p, '/') != NULL) {
		err("proc id contains multiple '/': %s", id);
		return -1;
	}

	/* Ensure the prefix is ok */
	if (!path_has_prefix(p, prefix)) {
		err("proc name must start with '%s': %s", prefix, id);
		return -1;
	}

	if (snprintf(proc->id, PATH_MAX, "%s", id) >= PATH_MAX) {
		err("proc id too long: %s", id);
		return -1;
	}

	return 0;
}

int
proc_init(struct proc *proc, const char *id, int pid)
{
	memset(proc, 0, sizeof(struct proc));

	if (set_id(proc, id) != 0) {
		err("cannot set process id");
		return -1;
	}

	proc->pid = pid;

	return 0;
}

static int
check_metadata_version(JSON_Object *meta)
{
	JSON_Value *version_val = json_object_get_value(meta, "version");
	if (version_val == NULL) {
		err("missing attribute \"version\"");
		return -1;
	}

	int version = (int) json_number(version_val);

	if (version != OVNI_METADATA_VERSION) {
		err("metadata version mismatch %d (expected %d)",
				version, OVNI_METADATA_VERSION);
		return -1;
	}

	JSON_Value *mversion_val = json_object_get_value(meta, "model_version");
	if (mversion_val == NULL) {
		err("missing attribute \"model_version\"");
		return -1;
	}

	const char *mversion = json_string(mversion_val);
	if (strcmp(mversion, OVNI_MODEL_VERSION) != 0) {
		err("model version mismatch '%s' (expected '%s')",
				mversion, OVNI_MODEL_VERSION);
		return -1;
	}

	return 0;
}

static int
load_attributes(struct proc *proc, JSON_Object *meta)
{
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

	return 0;
}

int
proc_load_metadata(struct proc *proc, JSON_Object *meta)
{
	if (proc->metadata_loaded) {
		err("process %s already loaded metadata", proc->id);
		return -1;
	}

	if (load_attributes(proc, meta) != 0) {
		err("cannot load attributes for process %s", proc->id);
		return -1;
	}

	proc->metadata_loaded = 1;

	return 0;
}

struct thread *
proc_find_thread(struct proc *proc, int tid)
{
	struct thread *th;
	DL_FOREACH2(proc->threads, th, lnext) {
		if (th->tid == tid)
			return th;
	}
	return NULL;
}

int
proc_get_pid(struct proc *proc)
{
	return proc->pid;
}
