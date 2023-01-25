/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu_system.h"
#include "utlist.h"
#include <errno.h>

void
proc_init(struct proc *proc, struct loom *loom, pid_t pid)
{
	memset(proc, 0, sizeof(struct proc));

	if (snprintf(proc->name, PATH_MAX, "%s", name) >= PATH_MAX)
		die("new_proc: name too long: %s\n", name);

	if (snprintf(proc->relpath, PATH_MAX, "%s/%s", loom->name, proc->name) >= PATH_MAX)
		die("new_proc: relative path too long: %s/%s", loom->name, proc->name);

	if (snprintf(proc->path, PATH_MAX, "%s/%s", tracedir, proc->relpath) >= PATH_MAX)
		die("new_proc: path too long: %s/%s", tracedir, proc->relpath);

	proc->pid = pid;
	proc->loom = loom;
	proc->id = proc->relpath;

	err("new proc '%s'\n", proc->id);
}

static int
check_proc_metadata(JSON_Value *meta_val, const char *path)
{
	JSON_Object *meta = json_value_get_object(meta_val);
	if (meta == NULL) {
		err("check_proc_metadata: json_value_get_object() failed: %s\n",
				path);
		return -1;
	}

	JSON_Value *version_val = json_object_get_value(meta, "version");
	if (version_val == NULL) {
		err("check_proc_metadata: missing attribute \"version\": %s\n",
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

int
proc_load_metadata(struct emu_proc *proc, char *metadata_file)
{
	if (proc->meta != NULL) {
		err("proc_load_metadata: process '%s' already has metadata\n",
				proc->id);
		return -1;
	}

	proc->meta = json_parse_file_with_comments(metadata_file);
	if (proc->meta == NULL) {
		err("proc_load_metadata: failed to load metadata: %s\n",
				metadata_file);
		return -1;
	}

	if (check_proc_metadata(proc->meta, path) != 0) {
		err("load_proc_metadata: invalid metadata: %s\n",
				metadata_file);
		return -1;
	}

	/* The appid is populated from the metadata */
	if (load_proc_attributes(proc, path) != 0) {
		err("load_proc_metadata: invalid attributes: %s\n",
				metadata_file);
		return -1;
	}

	return 0;
}

struct thread *
proc_find_thread(struct proc *proc, pid_t tid)
{
	struct thread *th;
	DL_FOREACH2(proc->threads, th, lnext) {
		if (t->tid == tid)
			return t;
	}
	return NULL;
}
