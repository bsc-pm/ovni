/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "nanos6_priv.h"
#include <stddef.h>
#include <stdlib.h>
#include "chan.h"
#include "cpu.h"
#include "common.h"
#include "emu.h"
#include "emu_args.h"
#include "emu_prv.h"
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
#include "recorder.h"
#include "system.h"
#include "task.h"
#include "thread.h"
#include "track.h"
#include "value.h"

static const char model_name[] = "nanos6";
enum { model_id = '6' };

struct model_spec model_nanos6 = {
	.name = model_name,
	.model = model_id,
	.create  = model_nanos6_create,
//	.connect = model_nanos6_connect,
	.event   = model_nanos6_event,
	.probe   = model_nanos6_probe,
	.finish  = model_nanos6_finish,
};

/* ----------------- channels ------------------ */

static const char *chan_name[CH_MAX] = {
	[CH_TASKID]    = "taskid",
	[CH_TYPE]      = "task_type",
	[CH_SUBSYSTEM] = "subsystem",
	[CH_RANK]      = "rank",
	[CH_THREAD]    = "thread_type",
	[CH_IDLE]      = "idle",
};

static const int chan_stack[CH_MAX] = {
	[CH_SUBSYSTEM] = 1,
	[CH_THREAD] = 1,
	[CH_IDLE] = 1,
};

static const int chan_dup[CH_MAX] = {
	[CH_TYPE] = 1,
	[CH_RANK] = 1,
};


/* ----------------- pvt ------------------ */

static const int pvt_type[] = {
	[CH_TASKID]    = PRV_NANOS6_TASKID,
	[CH_TYPE]      = PRV_NANOS6_TYPE,
	[CH_SUBSYSTEM] = PRV_NANOS6_SUBSYSTEM,
	[CH_RANK]      = PRV_NANOS6_RANK,
	[CH_THREAD]    = PRV_NANOS6_THREAD,
	[CH_IDLE]      = PRV_NANOS6_IDLE,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_TASKID]    = "Nanos6 task ID",
	[CH_TYPE]      = "Nanos6 task type",
	[CH_SUBSYSTEM] = "Nanos6 subsystem",
	[CH_RANK]      = "Nanos6 task MPI rank",
	[CH_THREAD]    = "Nanos6 thread type",
	[CH_IDLE]      = "Nanos6 idle state",
};

static const struct pcf_value_label nanos6_ss_values[] = {
	{ ST_TASK_BODY,        "Task: Running body" },
	{ ST_TASK_CREATING,    "Task: Creating" },
	{ ST_TASK_SUBMIT,      "Task: Submitting" },
	{ ST_TASK_SPAWNING,    "Task: Spawning function" },
	{ ST_TASK_FOR,         "Task: Running task for" },
	{ ST_SCHED_SERVING,    "Scheduler: Serving tasks" },
	{ ST_SCHED_ADDING,     "Scheduler: Adding ready tasks" },
	{ ST_SCHED_PROCESSING, "Scheduler: Processing ready tasks" },
	{ ST_DEP_REG,          "Dependency: Registering" },
	{ ST_DEP_UNREG,        "Dependency: Unregistering" },
	{ ST_BLK_TASKWAIT,     "Blocking: Taskwait" },
	{ ST_BLK_BLOCKING,     "Blocking: Blocking current task" },
	{ ST_BLK_UNBLOCKING,   "Blocking: Unblocking remote task" },
	{ ST_BLK_WAITFOR,      "Blocking: Wait for deadline" },
	{ ST_HANDLING_TASK,    "Worker: Handling task" },
	{ ST_WORKER_LOOP,      "Worker: Looking for work" },
	{ ST_WORKER_INIT,      "Worker: Starting" },
	{ ST_SWITCH_TO,        "Worker: Switching to another thread" },
	{ ST_MIGRATE,          "Worker: Migrating CPU" },
	{ ST_SUSPEND,          "Worker: Suspending thread" },
	{ ST_RESUME,           "Worker: Resuming another thread" },
	{ ST_ALLOCATING,       "Memory: Allocating" },
	{ ST_FREEING,          "Memory: Freeing" },

	{ EV_SCHED_SEND,       "EV Scheduler: Send task" },
	{ EV_SCHED_RECV,       "EV Scheduler: Recv task" },
	{ EV_SCHED_SELF,       "EV Scheduler: Self-assign task" },
	{ EV_CPU_IDLE,         "EV CPU: Becomes idle" },
	{ EV_CPU_ACTIVE,       "EV CPU: Becomes active" },
	{ EV_SIGNAL,           "EV Worker: Waking another thread" },
	{ -1, NULL },
};

static const struct pcf_value_label nanos6_thread_type[] = {
	{ ST_TH_EXTERNAL,   "External" },
	{ ST_TH_WORKER,     "Worker" },
	{ ST_TH_LEADER,     "Leader" },
	{ ST_TH_MAIN,       "Main" },
	{ -1, NULL },
};

static const struct pcf_value_label nanos6_worker_idle[] = {
	{ ST_WORKER_IDLE,   "Idle" },
	{ ST_WORKER_BUSY,   "Busy" },
	{ -1, NULL },
};

static const struct pcf_value_label (*pcf_labels[CH_MAX])[] = {
	[CH_SUBSYSTEM] = &nanos6_ss_values,
	[CH_THREAD]    = &nanos6_thread_type,
	[CH_IDLE]      = &nanos6_worker_idle,
};

static const long prv_flags[CH_MAX] = {
	[CH_TASKID]    = PRV_SKIPDUP,
	[CH_TYPE]      = PRV_EMITDUP, /* Switch to task of same type */
	[CH_SUBSYSTEM] = PRV_SKIPDUP,
	[CH_RANK]      = PRV_EMITDUP, /* Switch to task of same rank */
	[CH_THREAD]    = PRV_SKIPDUP,
	[CH_IDLE]      = PRV_SKIPDUP,
};

static const struct model_pvt_spec pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
	.flags = prv_flags,
};

/* ----------------- tracking ------------------ */

static const int th_track[CH_MAX] = {
	[CH_TASKID]    = TRACK_TH_RUN,
	[CH_TYPE]      = TRACK_TH_RUN,
	[CH_SUBSYSTEM] = TRACK_TH_ACT,
	[CH_RANK]      = TRACK_TH_RUN,
	[CH_THREAD]    = TRACK_TH_ANY,
	[CH_IDLE]      = TRACK_TH_RUN,
};

static const int cpu_track[CH_MAX] = {
	[CH_TASKID]    = TRACK_TH_RUN,
	[CH_TYPE]      = TRACK_TH_RUN,
	[CH_SUBSYSTEM] = TRACK_TH_RUN,
	[CH_RANK]      = TRACK_TH_RUN,
	[CH_THREAD]    = TRACK_TH_RUN,
	[CH_IDLE]      = TRACK_TH_RUN,
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
	.size = sizeof(struct nanos6_cpu),
	.chan = &cpu_chan,
	.model = &model_nanos6,
};

static const struct model_thread_spec th_spec = {
	.size = sizeof(struct nanos6_thread),
	.chan = &th_chan,
	.model = &model_nanos6,
};

/* ----------------------------------------------------- */

int
model_nanos6_probe(struct emu *emu)
{
	if (emu->system.nthreads == 0)
		return 1;

	return 0;
}

static int
init_proc(struct proc *sysproc)
{
	struct nanos6_proc *proc = calloc(1, sizeof(struct nanos6_proc));
	if (proc == NULL) {
		err("calloc failed:");
		return -1;
	}

	extend_set(&sysproc->ext, model_id, proc);

	return 0;
}

int
model_nanos6_create(struct emu *emu)
{
	struct system *sys = &emu->system;

	if (model_thread_create(emu, &th_spec) != 0) {
		err("model_thread_init failed");
		return -1;
	}

	if (model_cpu_create(emu, &cpu_spec) != 0) {
		err("model_cpu_init failed");
		return -1;
	}

	/* Init task stack thread pointer */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		struct nanos6_thread *th = EXT(t, model_id);
		th->task_stack.thread = t;
	}

	for (struct proc *p = sys->procs; p; p = p->gnext) {
		if (init_proc(p) != 0) {
			err("init_proc failed");
			return -1;
		}
	}

	struct nanos6_emu *e = calloc(1, sizeof(struct nanos6_emu));
	if (e == NULL) {
		err("calloc failed:");
		return -1;
	}

	extend_set(&emu->ext, model_id, e);

	if (model_nanos6_breakdown_create(emu) != 0) {
		err("model_nanos6_breakdown_connect failed");
		return -1;
	}

	return 0;
}

int
model_nanos6_connect(struct emu *emu)
{
	if (model_thread_connect(emu, &th_spec) != 0) {
		err("model_thread_connect failed");
		return -1;
	}

	if (model_cpu_connect(emu, &cpu_spec) != 0) {
		err("model_cpu_connect failed");
		return -1;
	}

	if (model_nanos6_breakdown_connect(emu) != 0) {
		err("model_nanos6_breakdown_connect failed");
		return -1;
	}

	for (struct thread *th = emu->system.threads; th; th = th->gnext) {
		struct nanos6_thread *mth = EXT(th, model_id);
		struct chan *idle = &mth->m.ch[CH_IDLE];
		/* By default set all threads as Busy */
		if (chan_push(idle, value_int64(ST_WORKER_BUSY)) != 0) {
			err("chan_push idle failed");
			return -1;
		}
		struct chan *ss = &mth->m.ch[CH_SUBSYSTEM];
		/* And push initial subsystem to worker init */
		if (chan_push(ss, value_int64(ST_WORKER_INIT)) != 0) {
			err("chan_push idle failed");
			return -1;
		}
	}

	for (struct cpu *cpu = emu->system.cpus; cpu; cpu = cpu->next) {
		struct nanos6_cpu *mcpu = EXT(cpu, model_id);
		struct mux *mux = &mcpu->m.track[CH_IDLE].mux;
		/* Emit Idle when a CPU has no idle threads */
		mux_set_default(mux, value_int64(ST_WORKER_IDLE));
	}

	return 0;
}

/* TODO: Automatically check all stack channels at the end */
static int
end_lint(struct emu *emu)
{
	/* Only run the check if we finished the complete trace */
	if (!emu->finished)
		return 0;

	struct system *sys = &emu->system;

	/* Ensure we run out of subsystem states */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		struct nanos6_thread *th = EXT(t, model_id);
		struct chan *ch = &th->m.ch[CH_SUBSYSTEM];
		int stacked = ch->data.stack.n;
		if (stacked > 1) {
			struct value top;
			if (chan_read(ch, &top) != 0) {
				err("chan_read failed for subsystem");
				return -1;
			}

			err("thread %d ended with %d stacked nanos6 subsystems",
					t->tid, stacked);
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
	long typeid = pvt_type[CH_TYPE];
	struct pcf_type *pcftype = pcf_find_type(pcf, typeid);

	for (struct proc *p = sys->procs; p; p = p->gnext) {
		struct nanos6_proc *proc = EXT(p, model_id);
		struct task_info *info = &proc->task_info;
		if (task_create_pcf_types(pcftype, info->types) != 0) {
			err("task_create_pcf_types failed");
			return -1;
		}
	}

	return 0;
}

int
model_nanos6_finish(struct emu *emu)
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

	if (model_nanos6_breakdown_finish(emu, pcf_labels) != 0) {
		err("model_nanos6_breakdown_finish failed");
		return -1;
	}

	/* When running in linter mode perform additional checks */
	if (emu->args.linter_mode && end_lint(emu) != 0) {
		err("end_lint failed");
		return -1;
	}

	return 0;
}
