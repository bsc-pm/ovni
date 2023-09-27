/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "openmp_priv.h"
#include <stddef.h>
#include "chan.h"
#include "common.h"
#include "emu.h"
#include "emu_args.h"
#include "emu_prv.h"
#include "ev_spec.h"
#include "extend.h"
#include "model.h"
#include "model_chan.h"
#include "model_cpu.h"
#include "model_pvt.h"
#include "model_thread.h"
#include "pv/pcf.h"
#include "pv/prv.h"
#include "system.h"
#include "thread.h"
#include "track.h"
#include "value.h"

static const char model_name[] = "openmp";
enum { model_id = 'P' };

static struct ev_decl model_evlist[] = {
	PAIR_E("PA[", "PA]", "the attached state")

	PAIR_E("PBj", "PBJ", "a join barrier")
	PAIR_E("PBb", "PBB", "a barrier")
	PAIR_E("PBt", "PBT", "a tasking barrier")
	PAIR_E("PBs", "PBS", "a spin wait")

	PAIR_B("PWs", "PWS", "static for")
	PAIR_B("PWd", "PWD", "dynamic for init")
	PAIR_B("PWc", "PWC", "dynamic for chunk")
	PAIR_B("PWi", "PWI", "single")

	PAIR_B("PTr", "PTR", "releasing task dependencies")
	PAIR_B("PTw", "PTW", "waiting for taskwait dependencies")
	PAIR_B("PT[", "PT]", "invoking a task")
	PAIR_B("PTi", "PTI", "invoking an if0 task")
	PAIR_B("PTa", "PTA", "task allocation")
	PAIR_B("PTs", "PTS", "scheduling a task")
	PAIR_E("PTt", "PTT", "a taskwait")
	PAIR_E("PTy", "PTY", "a taskyield")
	PAIR_B("PTd", "PTD", "duplicating a task")
	PAIR_B("PTc", "PTC", "checking task dependencies")
	PAIR_E("PTg", "PTG", "a taskgroup")

	{ NULL, NULL },
};

struct model_spec model_openmp = {
	.name = model_name,
	.version = "1.1.0",
	.evlist  = model_evlist,
	.model = model_id,
	.create  = model_openmp_create,
	.connect = model_openmp_connect,
	.event   = model_openmp_event,
	.probe   = model_openmp_probe,
	.finish  = model_openmp_finish,
};

/* ----------------- channels ------------------ */

static const char *chan_name[CH_MAX] = {
	[CH_SUBSYSTEM] = "subsystem",
};

static const int chan_stack[CH_MAX] = {
	[CH_SUBSYSTEM] = 1,
};

/* ----------------- pvt ------------------ */

static const int pvt_type[CH_MAX] = {
	[CH_SUBSYSTEM] = PRV_OPENMP_SUBSYSTEM,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_SUBSYSTEM] = "OpenMP subsystem",
};

static const struct pcf_value_label openmp_subsystem_values[] = {
	{ ST_ATTACHED,          "Attached" },
	{ ST_JOIN_BARRIER,      "Join barrier" },
	{ ST_BARRIER,           "Barrier" },
	{ ST_TASKING_BARRIER,   "Tasking barrier" },
	{ ST_SPIN_WAIT,         "Spin wait" },
	{ ST_FOR_STATIC,        "For static" },
	{ ST_FOR_DYNAMIC_INIT,  "For dynamic init" },
	{ ST_FOR_DYNAMIC_CHUNK, "For dynamic chunk" },
	{ ST_SINGLE,            "Single" },
	{ ST_RELEASE_DEPS,      "Release deps" },
	{ ST_TASKWAIT_DEPS,     "Taskwait deps" },
	{ ST_INVOKE_TASK,       "Invoke task" },
	{ ST_INVOKE_TASK_IF0,   "Invoke task if0" },
	{ ST_TASK_ALLOC,        "Task alloc" },
	{ ST_TASK_SCHEDULE,     "Task schedule" },
	{ ST_TASKWAIT,          "Taskwait" },
	{ ST_TASKYIELD,         "Taskyield" },
	{ ST_TASK_DUP_ALLOC,    "Task dup alloc" },
	{ ST_CHECK_DEPS,        "Check deps" },
	{ ST_TASKGROUP,         "Taskgroup" },
	{ -1, NULL },
};

static const struct pcf_value_label *pcf_labels[CH_MAX] = {
	[CH_SUBSYSTEM] = openmp_subsystem_values,
};

static const long prv_flags[CH_MAX] = {
	[CH_SUBSYSTEM] = PRV_SKIPDUP,
};

static const struct model_pvt_spec pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
	.flags = prv_flags,
};

/* ----------------- tracking ------------------ */

static const int th_track[CH_MAX] = {
	[CH_SUBSYSTEM] = TRACK_TH_RUN,
};

static const int cpu_track[CH_MAX] = {
	[CH_SUBSYSTEM] = TRACK_TH_RUN,
};

/* ----------------- chan_spec ------------------ */

static const struct model_chan_spec th_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.pvt = &pvt_spec,
	.track = th_track,
};

static const struct model_chan_spec cpu_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.pvt = &pvt_spec,
	.track = cpu_track,
};

/* ----------------- models ------------------ */

static const struct model_cpu_spec cpu_spec = {
	.size = sizeof(struct openmp_cpu),
	.chan = &cpu_chan,
	.model = &model_openmp,
};

static const struct model_thread_spec th_spec = {
	.size = sizeof(struct openmp_thread),
	.chan = &th_chan,
	.model = &model_openmp,
};

/* ----------------------------------------------------- */

int
model_openmp_probe(struct emu *emu)
{
	return model_version_probe(&model_openmp, emu);
}

int
model_openmp_create(struct emu *emu)
{
	if (model_thread_create(emu, &th_spec) != 0) {
		err("model_thread_init failed");
		return -1;
	}

	if (model_cpu_create(emu, &cpu_spec) != 0) {
		err("model_cpu_init failed");
		return -1;
	}

	return 0;
}

int
model_openmp_connect(struct emu *emu)
{
	if (model_thread_connect(emu, &th_spec) != 0) {
		err("model_thread_connect failed");
		return -1;
	}

	if (model_cpu_connect(emu, &cpu_spec) != 0) {
		err("model_cpu_connect failed");
		return -1;
	}

	return 0;
}

static int
end_lint(struct emu *emu)
{
	/* Only run the check if we finished the complete trace */
	if (!emu->finished)
		return 0;

	struct system *sys = &emu->system;

	/* Ensure we run out of function states */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		struct openmp_thread *th = EXT(t, model_id);
		struct chan *ch = &th->m.ch[CH_SUBSYSTEM];
		int stacked = ch->data.stack.n;
		if (stacked > 0) {
			struct value top;
			if (chan_read(ch, &top) != 0) {
				err("chan_read failed for function");
				return -1;
			}

			err("thread %d ended with %d stacked openmp functions",
					t->tid, stacked);
			return -1;
		}
	}

	return 0;
}

int
model_openmp_finish(struct emu *emu)
{
	/* When running in linter mode perform additional checks */
	if (emu->args.linter_mode && end_lint(emu) != 0) {
		err("end_lint failed");
		return -1;
	}

	return 0;
}
