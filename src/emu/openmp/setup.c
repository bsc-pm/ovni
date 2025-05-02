/* Copyright (c) 2023-2025 Barcelona Supercomputing Center (BSC)
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
#include "proc.h"
#include "pv/pcf.h"
#include "pv/prv.h"
#include "pv/pvt.h"
#include "system.h"
#include "thread.h"
#include "track.h"
#include "value.h"

static const char model_name[] = "openmp";
enum { model_id = 'P' };

static struct ev_decl model_evlist[] = {
	PAIR_B("PBb", "PBB", "plain barrier")
	PAIR_B("PBj", "PBJ", "join barrier")
	PAIR_B("PBf", "PBF", "fork barrier")
	PAIR_B("PBt", "PBT", "tasking barrier")
	PAIR_B("PBs", "PBS", "spin wait")

	PAIR_B("PIa", "PIA", "critical acquiring")
	PAIR_B("PIr", "PIR", "critical releasing")
	PAIR_B("PI[", "PI]", "critical section")

	PAIR_B("PWd", "PWD", "distribute")
	PAIR_B("PWy", "PWY", "dynamic for init")
	PAIR_B("PWc", "PWC", "dynamic for chunk")
	PAIR_B("PWs", "PWS", "static for")
	PAIR_B("PWe", "PWE", "section")
	PAIR_B("PWi", "PWI", "single")

	PAIR_B("PTa", "PTA", "task allocation")
	PAIR_B("PTc", "PTC", "checking task dependencies")
	PAIR_B("PTd", "PTD", "duplicating a task")
	PAIR_B("PTr", "PTR", "releasing task dependencies")
	PAIR_B("PT[", "PT]", "running a task")
	PAIR_B("PTi", "PTI", "running an if0 task")
	PAIR_B("PTs", "PTS", "scheduling a task")
	PAIR_B("PTg", "PTG", "a taskgroup")
	PAIR_B("PTt", "PTT", "a taskwait")
	PAIR_B("PTw", "PTW", "waiting for taskwait dependencies")
	PAIR_B("PTy", "PTY", "a taskyield")

	PAIR_E("PA[", "PA]", "the attached state")

	PAIR_B("PMi", "PMI", "microtask internal")
	PAIR_B("PMu", "PMU", "microtask user code")

	PAIR_B("PH[", "PH]", "worker loop")

	PAIR_B("PCf", "PCF", "fork call")
	PAIR_B("PCi", "PCI", "initialization")

	/* Task or worksharing type */
	{ "POc+(u32 typeid, str label)", "creates a type %{typeid} with label \"%{label}\"" },

	{ "PPc(u32 taskid, u32 typeid)", "creates the task %{taskid} with type %{typeid}" },
	{ "PPx(u32 taskid)", "executes the task %{taskid}" },
	{ "PPe(u32 taskid)", "ends the task %{taskid}" },

	{ "PQx(u32 typeid)", "begins worksharing with type %{typeid}" },
	{ "PQe(u32 typeid)", "ends worksharing with type %{typeid}" },

	{ NULL, NULL },
};

struct model_spec model_openmp = {
	.name = model_name,
	.version = "1.2.1",
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
	[CH_LABEL]     = "label",
	[CH_TASKID]    = "task ID",
};

static const int chan_stack[CH_MAX] = {
	[CH_SUBSYSTEM] = 1,
	[CH_LABEL]     = 1,
	[CH_TASKID]    = 1,
};

static const int chan_dup[CH_MAX] = {
	[CH_SUBSYSTEM] = 1,
	[CH_LABEL]     = 1, /* Two tasks nested with same type */
	[CH_TASKID]    = 1,
};

/* ----------------- pvt ------------------ */

static const int pvt_type[CH_MAX] = {
	[CH_SUBSYSTEM] = PRV_OPENMP_SUBSYSTEM,
	[CH_LABEL]     = PRV_OPENMP_LABEL,
	[CH_TASKID]    = PRV_OPENMP_TASKID,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_SUBSYSTEM] = "OpenMP subsystem",
	[CH_LABEL]     = "OpenMP label",
	[CH_TASKID]    = "OpenMP task ID",
};

static const struct pcf_value_label openmp_subsystem_values[] = {
	/* Work-distribution */
	{ ST_WD_DISTRIBUTE,          "Work-distribution: Distribute" },
	{ ST_WD_FOR_DYNAMIC_CHUNK,   "Work-distribution: Dynamic for chunk" },
	{ ST_WD_FOR_DYNAMIC_INIT,    "Work-distribution: Dynamic for initialization" },
	{ ST_WD_FOR_STATIC,          "Work-distribution: Static for chunk" },
	{ ST_WD_SECTION,             "Work-distribution: Section" },
	{ ST_WD_SINGLE,              "Work-distribution: Single" },
	/* Task */
	{ ST_TASK_ALLOC,             "Task: Allocation" },
	{ ST_TASK_CHECK_DEPS,        "Task: Check deps" },
	{ ST_TASK_DUP_ALLOC,         "Task: Duplicating" },
	{ ST_TASK_RELEASE_DEPS,      "Task: Releasing deps" },
	{ ST_TASK_RUN,               "Task: Running task" },
	{ ST_TASK_RUN_IF0,           "Task: Running task if0" },
	{ ST_TASK_SCHEDULE,          "Task: Scheduling" },
	{ ST_TASK_TASKGROUP,         "Task: Taskgroup" },
	{ ST_TASK_TASKWAIT,          "Task: Taskwait" },
	{ ST_TASK_TASKWAIT_DEPS,     "Task: Taskwait deps" },
	{ ST_TASK_TASKYIELD,         "Task: Taskyield" },
	/* Critical */
	{ ST_CRITICAL_ACQ,           "Critical: Acquiring" },
	{ ST_CRITICAL_REL,           "Critical: Releasing" },
	{ ST_CRITICAL_SECTION,       "Critical: Section" },
	/* Barrier */
	{ ST_BARRIER_FORK,           "Barrier: Fork" },
	{ ST_BARRIER_JOIN,           "Barrier: Join" },
	{ ST_BARRIER_PLAIN,          "Barrier: Plain" },
	{ ST_BARRIER_TASK,           "Barrier: Task" },
	{ ST_BARRIER_SPIN_WAIT,      "Barrier: Spin wait" },
	/* Runtime */
	{ ST_RT_ATTACHED,            "Runtime: Attached" },
	{ ST_RT_FORK_CALL,           "Runtime: Fork call" },
	{ ST_RT_INIT,                "Runtime: Initialization" },
	{ ST_RT_MICROTASK_INTERNAL,  "Runtime: Internal microtask" },
	{ ST_RT_MICROTASK_USER,      "Runtime: User microtask" },
	{ ST_RT_WORKER_LOOP,         "Runtime: Worker main loop" },
	{ -1, NULL },
};

static const struct pcf_value_label *pcf_labels[CH_MAX] = {
	[CH_SUBSYSTEM] = openmp_subsystem_values,
};

static const long prv_flags[CH_MAX] = {
	[CH_SUBSYSTEM] = PRV_SKIPDUPNULL,
	[CH_LABEL]     = PRV_SKIPDUPNULL,
	[CH_TASKID]    = PRV_SKIPDUPNULL,
};

static const struct model_pvt_spec pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
	.flags = prv_flags,
};

/* ----------------- tracking ------------------ */

static const int th_track[CH_MAX] = {
	[CH_SUBSYSTEM] = TRACK_TH_ACT,
	[CH_LABEL]     = TRACK_TH_ACT,
	[CH_TASKID]    = TRACK_TH_ACT,
};

static const int cpu_track[CH_MAX] = {
	[CH_SUBSYSTEM] = TRACK_TH_RUN,
	[CH_LABEL]     = TRACK_TH_RUN,
	[CH_TASKID]    = TRACK_TH_RUN,
};

/* ----------------- chan_spec ------------------ */

static const struct model_chan_spec th_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.ch_dup = chan_dup,
	.pvt = &pvt_spec,
	.track = th_track,
};

static const struct model_chan_spec cpu_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.ch_dup = chan_dup,
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

static int
init_proc(struct proc *sysproc)
{
	struct openmp_proc *proc = calloc(1, sizeof(struct openmp_proc));
	if (proc == NULL) {
		err("calloc failed:");
		return -1;
	}

	extend_set(&sysproc->ext, model_id, proc);

	return 0;
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

	struct system *sys = &emu->system;

	for (struct proc *p = sys->procs; p; p = p->gnext) {
		if (init_proc(p) != 0) {
			err("init_proc failed");
			return -1;
		}
	}

	struct openmp_emu *e = calloc(1, sizeof(struct openmp_emu));
	if (e == NULL) {
		err("calloc failed:");
		return -1;
	}

	extend_set(&emu->ext, model_id, e);

	if (model_openmp_breakdown_create(emu) != 0) {
		err("model_openmp_breakdown_create failed");
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

	if (model_openmp_breakdown_connect(emu) != 0) {
		err("model_openmp_breakdown_connect failed");
		return -1;
	}

	return 0;
}

static int
create_pcf_type(struct system *sys, struct pcf *pcf, long typeid)
{
	struct pcf_type *pcftype = pcf_find_type(pcf, (int) typeid);

	for (struct proc *p = sys->procs; p; p = p->gnext) {
		struct openmp_proc *proc = EXT(p, model_id);
		struct task_info *info = &proc->task_info;
		if (task_create_pcf_types(pcftype, info->types) != 0) {
			err("task_create_pcf_types failed");
			return -1;
		}
	}

	return 0;
}

static int
finish_pvt(struct emu *emu, const char *name)
{
	struct system *sys = &emu->system;

	/* Emit task types for all channel types and processes */
	struct pvt *pvt = recorder_find_pvt(&emu->recorder, name);
	if (pvt == NULL) {
		err("cannot find pvt with name '%s'", name);
		return -1;
	}

	struct pcf *pcf = pvt_get_pcf(pvt);
	if (create_pcf_type(sys, pcf, pvt_type[CH_LABEL]) != 0) {
		err("create_pcf_type failed");
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
	/* Fill task types */
	if (finish_pvt(emu, "thread") != 0) {
		err("finish_pvt thread failed");
		return -1;
	}

	if (finish_pvt(emu, "cpu") != 0) {
		err("finish_pvt cpu failed");
		return -1;
	}

	if (model_openmp_breakdown_finish(emu, pcf_labels) != 0) {
		err("model_openmp_breakdown_finish failed");
		return -1;
	}

	/* When running in linter mode perform additional checks */
	if (emu->args.linter_mode && end_lint(emu) != 0) {
		err("end_lint failed");
		return -1;
	}

	return 0;
}
