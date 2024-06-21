/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "nosv_priv.h"
#include <stddef.h>
#include <stdlib.h>
#include "chan.h"
#include "cpu.h"
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
#include "recorder.h"
#include "system.h"
#include "task.h"
#include "thread.h"
#include "track.h"
#include "value.h"

static const char model_name[] = "nosv";
enum { model_id = 'V' };

static struct ev_decl model_evlist[] = {
	{ "VTc(u32 taskid, u32 typeid)", "creates task %{taskid} with type %{typeid}" },
	{ "VTC(u32 taskid, u32 typeid)", "creates parallel task %{taskid} with type %{typeid}" },
	{ "VTx(u32 taskid, u32 bodyid)", "executes the task %{taskid} with bodyid %{bodyid}" },
	{ "VTe(u32 taskid, u32 bodyid)", "ends the task %{taskid} with bodyid %{bodyid}" },
	{ "VTp(u32 taskid, u32 bodyid)", "pauses the task %{taskid} with bodyid %{bodyid}" },
	{ "VTr(u32 taskid, u32 bodyid)", "resumes the task %{taskid} with bodyid %{bodyid}" },

	{ "VYc+(u32 typeid, str label)", "creates task type %{typeid} with label \"%{label}\"" },

	{ "VSr", "receives a task from another thread" },
	{ "VSs", "sends a task to another thread" },
	{ "VS@", "self assigns itself a task" },
	{ "VSh", "enters the hungry state, waiting for work" },
	{ "VSf", "is no longer hungry" },
	PAIR_E("VS[", "VS]", "scheduler server mode")

	PAIR_S("VU[", "VU]", "submitting a task")
	PAIR_S("VMa", "VMA", "allocating memory")
	PAIR_S("VMf", "VMF", "freeing memory")

	PAIR_E("VAr", "VAR", "nosv_create()")
	PAIR_E("VAd", "VAD", "nosv_destroy()")
	PAIR_E("VAs", "VAS", "nosv_submit()")
	PAIR_E("VAp", "VAP", "nosv_pause()")
	PAIR_E("VAy", "VAY", "nosv_yield()")
	PAIR_E("VAw", "VAW", "nosv_waitfor()")
	PAIR_E("VAc", "VAC", "nosv_schedpoint()")
	PAIR_E("VAa", "VAA", "nosv_attach()")
	PAIR_E("VAe", "VAE", "nosv_detach()")
	PAIR_E("VAl", "VAL", "nosv_mutex_lock()")
	PAIR_E("VAt", "VAT", "nosv_mutex_trylock()")
	PAIR_E("VAu", "VAU", "nosv_mutex_unlock()")
	PAIR_E("VAb", "VAB", "nosv_barrier_wait()")

	/* FIXME: VHA and VHa are not subsystems */
	{ "VHa", "enters nosv_attach()" },
	{ "VHA", "leaves nosv_dettach()" },

	PAIR_B("VHw", "VHW", "execution as worker")
	PAIR_B("VHd", "VHD", "execution as delegate")

	{ "VPp", "sets progress state to Progressing" },
	{ "VPr", "sets progress state to Resting" },
	{ "VPa", "sets progress state to Absorbing" },

	{ NULL, NULL },
};

struct model_spec model_nosv = {
	.name    = model_name,
	.version = "2.3.0",
	.evlist  = model_evlist,
	.model   = model_id,
	.create  = model_nosv_create,
	.connect = model_nosv_connect,
	.event   = model_nosv_event,
	.probe   = model_nosv_probe,
	.finish  = model_nosv_finish,
};

/* ----------------- channels ------------------ */

static const char *chan_name[CH_MAX] = {
	[CH_BODYID]    = "bodyid",
	[CH_TASKID]    = "taskid",
	[CH_TYPE]      = "task_type",
	[CH_APPID]     = "appid",
	[CH_SUBSYSTEM] = "subsystem",
	[CH_RANK]      = "rank",
	[CH_IDLE]      = "idle",
};

static const int chan_stack[CH_MAX] = {
	[CH_SUBSYSTEM] = 1,
};

static const int chan_dup[CH_MAX] = {
	[CH_BODYID] = 1, /* Switch to another task with body 1 */
	[CH_APPID] = 1,
	[CH_TYPE] = 1,
	[CH_RANK] = 1,
	/* Can push twice ST_TASK_BODY if we run without subsystem
	 * events (level 2). */
	[CH_SUBSYSTEM] = 1,
};

/* ----------------- pvt ------------------ */

static const int pvt_type[CH_MAX] = {
	[CH_BODYID]    = PRV_NOSV_BODYID,
	[CH_TASKID]    = PRV_NOSV_TASKID,
	[CH_TYPE]      = PRV_NOSV_TYPE,
	[CH_APPID]     = PRV_NOSV_APPID,
	[CH_SUBSYSTEM] = PRV_NOSV_SUBSYSTEM,
	[CH_RANK]      = PRV_NOSV_RANK,
	[CH_IDLE]      = PRV_NOSV_IDLE,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_BODYID]      = "nOS-V task body ID",
	[CH_TASKID]      = "nOS-V task ID",
	[CH_TYPE]        = "nOS-V task type",
	[CH_APPID]       = "nOS-V task AppID",
	[CH_SUBSYSTEM]   = "nOS-V subsystem",
	[CH_RANK]        = "nOS-V task MPI rank",
	[CH_IDLE]        = "nOS-V idle state",
};

static const struct pcf_value_label nosv_ss_values[] = {
	{ ST_UNKNOWN_SS,       "Unknown subsystem" },
	{ ST_SCHED_HUNGRY,     "Scheduler: Hungry" },
	{ ST_SCHED_SERVING,    "Scheduler: Serving" },
	{ ST_SCHED_SUBMITTING, "Scheduler: Submitting" },
	{ ST_MEM_ALLOCATING,   "Memory: Allocating" },
	{ ST_MEM_FREEING,      "Memory: Freeing" },
	{ ST_TASK_BODY,        "Task: In body" },
	{ ST_API_CREATE,       "API: Create" },
	{ ST_API_DESTROY,      "API: Destroy" },
	{ ST_API_SUBMIT,       "API: Submit" },
	{ ST_API_PAUSE,        "API: Pause" },
	{ ST_API_YIELD,        "API: Yield" },
	{ ST_API_WAITFOR,      "API: Waitfor" },
	{ ST_API_SCHEDPOINT,   "API: Scheduling point" },
	{ ST_API_ATTACH,       "API: Attach" },
	{ ST_API_DETACH,       "API: Detach" },
	{ ST_API_MUTEX_LOCK,   "API: Mutex lock" },
	{ ST_API_MUTEX_TRYLOCK,"API: Mutex trylock" },
	{ ST_API_MUTEX_UNLOCK, "API: Mutex unlock" },
	{ ST_API_BARRIER_WAIT, "API: Barrier wait" },
	{ ST_WORKER,           "Thread: Worker" },
	{ ST_DELEGATE,         "Thread: Delegate" },
	{ EV_SCHED_SEND,       "EV Scheduler: Send task" },
	{ EV_SCHED_RECV,       "EV Scheduler: Recv task" },
	{ EV_SCHED_SELF,       "EV Scheduler: Self-assign task" },
	{ -1, NULL },
};

static const struct pcf_value_label nosv_worker_idle[] = {
	{ ST_PROGRESSING,   "Progressing" },
	{ ST_RESTING,       "Resting" },
	{ ST_ABSORBING,     "Absorbing noise" },
	{ -1, NULL },
};

static const struct pcf_value_label *pcf_labels[CH_MAX] = {
	[CH_SUBSYSTEM] = nosv_ss_values,
	[CH_IDLE]      = nosv_worker_idle,
};

static const long prv_flags[CH_MAX] = {
	[CH_BODYID]    = PRV_SKIPDUPNULL, /* Switch to different task, same bodyid */
	[CH_TASKID]    = PRV_SKIPDUPNULL, /* Switch to another body of the same task */
	[CH_TYPE]      = PRV_SKIPDUPNULL, /* Switch to task of same type */
	[CH_APPID]     = PRV_SKIPDUPNULL, /* Switch to task of same appid */
	[CH_SUBSYSTEM] = PRV_SKIPDUPNULL,
	[CH_RANK]      = PRV_SKIPDUPNULL, /* Switch to task of same rank */
	[CH_IDLE]      = PRV_SKIPDUPNULL,
};

static const struct model_pvt_spec pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
	.flags = prv_flags,
};

/* ----------------- tracking ------------------ */

static const int th_track[CH_MAX] = {
	[CH_BODYID]    = TRACK_TH_RUN,
	[CH_TASKID]    = TRACK_TH_RUN,
	[CH_TYPE]      = TRACK_TH_RUN,
	[CH_APPID]     = TRACK_TH_RUN,
	[CH_SUBSYSTEM] = TRACK_TH_ACT,
	[CH_RANK]      = TRACK_TH_RUN,
	[CH_IDLE]      = TRACK_TH_RUN,
};

static const int cpu_track[CH_MAX] = {
	[CH_BODYID]    = TRACK_TH_RUN,
	[CH_TASKID]    = TRACK_TH_RUN,
	[CH_TYPE]      = TRACK_TH_RUN,
	[CH_APPID]     = TRACK_TH_RUN,
	[CH_SUBSYSTEM] = TRACK_TH_RUN,
	[CH_RANK]      = TRACK_TH_RUN,
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
	.pvt = &pvt_spec,
	.track = cpu_track,
};

/* ----------------- models ------------------ */

static const struct model_thread_spec th_spec = {
	.size = sizeof(struct nosv_thread),
	.chan = &th_chan,
	.model = &model_nosv,
};

static const struct model_cpu_spec cpu_spec = {
	.size = sizeof(struct nosv_cpu),
	.chan = &cpu_chan,
	.model = &model_nosv,
};

/* ----------------------------------------------------- */

int
model_nosv_probe(struct emu *emu)
{
	return model_version_probe(&model_nosv, emu);
}

static int
init_proc(struct proc *sysproc)
{
	struct nosv_proc *proc = calloc(1, sizeof(struct nosv_proc));
	if (proc == NULL) {
		err("calloc failed:");
		return -1;
	}

	extend_set(&sysproc->ext, model_id, proc);

	return 0;
}

int
model_nosv_create(struct emu *emu)
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

	for (struct proc *p = sys->procs; p; p = p->gnext) {
		if (init_proc(p) != 0) {
			err("init_proc failed");
			return -1;
		}
	}

	struct nosv_emu *e = calloc(1, sizeof(struct nosv_emu));
	if (e == NULL) {
		err("calloc failed:");
		return -1;
	}

	extend_set(&emu->ext, model_id, e);

	if (model_nosv_breakdown_create(emu) != 0) {
		err("model_nosv_breakdown_create failed");
		return -1;
	}

	return 0;
}

int
model_nosv_connect(struct emu *emu)
{
	if (model_thread_connect(emu, &th_spec) != 0) {
		err("model_thread_connect failed");
		return -1;
	}

	if (model_cpu_connect(emu, &cpu_spec) != 0) {
		err("model_cpu_connect failed");
		return -1;
	}

	if (emu->args.breakdown && model_nosv_breakdown_connect(emu) != 0) {
		err("model_nosv_breakdown_connect failed");
		return -1;
	}

	for (struct thread *th = emu->system.threads; th; th = th->gnext) {
		struct nosv_thread *mth = EXT(th, model_id);
		struct chan *idle = &mth->m.ch[CH_IDLE];
		/* By default set all threads as Progressing */
		if (chan_set(idle, value_int64(ST_PROGRESSING)) != 0) {
			err("chan_push idle failed");
			return -1;
		}
	}

	for (struct cpu *cpu = emu->system.cpus; cpu; cpu = cpu->next) {
		struct nosv_cpu *mcpu = EXT(cpu, model_id);
		struct mux *mux = &mcpu->m.track[CH_IDLE].mux;
		/* Emit Resting when a CPU has no running threads */
		mux_set_default(mux, value_int64(ST_RESTING));
	}

	return 0;
}

/* TODO: Automatically check all stack channels at the end */
static int
end_lint(struct emu *emu)
{
	struct system *sys = &emu->system;

	/* Ensure we run out of subsystem states */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		struct nosv_thread *th = EXT(t, model_id);
		struct chan *ch = &th->m.ch[CH_SUBSYSTEM];
		int stacked = ch->data.stack.n;
		if (stacked > 0) {
			struct value top;
			if (chan_read(ch, &top) != 0) {
				err("chan_read failed for subsystem");
				return -1;
			}

			err("thread %d ended with %d stacked nosv subsystems",
					t->tid, stacked);
			return -1;
		}
	}

	return 0;
}

static int
finish_pvt(struct emu *emu, const char *name)
{
	/* Only run the check if we finished the complete trace */
	if (!emu->finished)
		return 0;

	struct system *sys = &emu->system;

	/* Emit task types for all channel types and processes */
	struct pvt *pvt = recorder_find_pvt(&emu->recorder, name);
	if (pvt == NULL) {
		err("cannot find pvt with name '%s'", name);
		return -1;
	}
	struct pcf *pcf = pvt_get_pcf(pvt);
	int typeid = pvt_type[CH_TYPE];
	struct pcf_type *pcftype = pcf_find_type(pcf, typeid);
	if (pcftype == NULL) {
		err("cannot find %s pcf type %d", name, typeid);
		return -1;
	}

	for (struct proc *p = sys->procs; p; p = p->gnext) {
		struct nosv_proc *proc = EXT(p, model_id);
		struct task_info *info = &proc->task_info;
		if (task_create_pcf_types(pcftype, info->types) != 0) {
			err("task_create_pcf_types failed");
			return -1;
		}
	}

	return 0;
}

int
model_nosv_finish(struct emu *emu)
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

	if (model_nosv_breakdown_finish(emu, pcf_labels) != 0) {
		err("model_nosv_breakdown_finish failed");
		return -1;
	}

	/* When running in linter mode perform additional checks */
	if (emu->args.linter_mode && end_lint(emu) != 0) {
		err("end_lint failed");
		return -1;
	}

	return 0;
}
