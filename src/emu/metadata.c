/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "metadata.h"
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "loom.h"
#include "ovni.h"
#include "parson.h"
#include "proc.h"
#include "stream.h"
#include "thread.h"

static int
check_version(JSON_Object *meta)
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

	return 0;
}

static int
has_cpus(JSON_Object *meta)
{
	/* Only check for the "ovni.loom_cpus" key, if it has zero
	 * elements is an error that will be reported later */
	if (json_object_dotget_array(meta, "ovni.loom_cpus") != NULL)
		return 1;

	return 0;
}

static int
load_cpus(struct loom *loom, JSON_Object *meta)
{
	JSON_Array *cpuarray = json_object_dotget_array(meta, "ovni.loom_cpus");
	if (cpuarray == NULL) {
		err("cannot find 'cpus' array");
		return -1;
	}

	size_t ncpus = json_array_get_count(cpuarray);
	if (ncpus == 0) {
		err("empty 'cpus' array in metadata");
		return -1;
	}

	if (loom->ncpus > 0) {
		err("loom %s already has cpus", loom->id);
		return -1;
	}

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

		cpu_init_begin(cpu, index, phyid, 0);

		if (loom_add_cpu(loom, cpu) != 0) {
			err("loom_add_cpu() failed");
			return -1;
		}
	}

	return 0;
}

int
metadata_load_proc(struct stream *s, struct loom *loom, struct proc *proc)
{
	JSON_Object *meta = stream_metadata(s);

	if (check_version(meta) != 0) {
		err("version check failed");
		return -1;
	}

	/* The appid is populated from the metadata */
	if (proc_load_metadata(proc, meta) != 0) {
		err("cannot load process attributes");
		return -1;
	}

	if (has_cpus(meta) && load_cpus(loom, meta) != 0) {
		err("cannot load loom cpus");
		return -1;
	}

	return 0;
}
