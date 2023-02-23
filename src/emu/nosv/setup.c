#include "nosv_priv.h"

static const char model_name[] = "nosv";
enum { model_id = 'V' };

struct model_spec model_nosv = {
	.name = model_name,
	.model = model_id,
	.create  = nosv_create,
//	.connect = nosv_connect,
	.event   = nosv_event,
	.probe   = nosv_probe,
	.finish  = nosv_finish,
};

/* ----------------- channels ------------------ */

static const char *chan_name[CH_MAX] = {
	[CH_TASKID]    = "taskid",
	[CH_TYPE]      = "task_type",
	[CH_APPID]     = "appid",
	[CH_SUBSYSTEM] = "subsystem",
	[CH_RANK]      = "rank",
};

static const int chan_stack[CH_MAX] = {
	[CH_SUBSYSTEM] = 1,
};

static const int chan_dup[CH_MAX] = {
	[CH_APPID] = 1,
	[CH_TYPE] = 1,
};

/* ----------------- pvt ------------------ */

static const int pvt_type[] = {
	[CH_TASKID]    = 10,
	[CH_TYPE]      = 11,
	[CH_APPID]     = 12,
	[CH_SUBSYSTEM] = 13,
	[CH_RANK]      = 14,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_TASKID]      = "nOS-V task ID",
	[CH_TYPE]        = "nOS-V task type",
	[CH_APPID]       = "nOS-V task AppID",
	[CH_SUBSYSTEM]   = "nOS-V subsystem",
	[CH_RANK]        = "nOS-V task MPI rank",
};

static const struct pcf_value_label nosv_ss_values[] = {
	{ ST_SCHED_HUNGRY,     "Scheduler: Hungry" },
	{ ST_SCHED_SERVING,    "Scheduler: Serving" },
	{ ST_SCHED_SUBMITTING, "Scheduler: Submitting" },
	{ ST_MEM_ALLOCATING,   "Memory: Allocating" },
	{ ST_MEM_FREEING,      "Memory: Freeing" },
	{ ST_TASK_RUNNING,     "Task: Running" },
	{ ST_API_SUBMIT,       "API: Submit" },
	{ ST_API_PAUSE,        "API: Pause" },
	{ ST_API_YIELD,        "API: Yield" },
	{ ST_API_WAITFOR,      "API: Waitfor" },
	{ ST_API_SCHEDPOINT,   "API: Scheduling point" },
	{ ST_ATTACH,           "Thread: Attached" },
	{ ST_WORKER,           "Thread: Worker" },
	{ ST_DELEGATE,         "Thread: Delegate" },
	{ EV_SCHED_SEND,       "EV Scheduler: Send task" },
	{ EV_SCHED_RECV,       "EV Scheduler: Recv task" },
	{ EV_SCHED_SELF,       "EV Scheduler: Self-assign task" },
	{ -1, NULL },
};

static const struct pcf_value_label (*pcf_labels[CH_MAX])[] = {
	[CH_SUBSYSTEM] = &nosv_ss_values,
};

/* ------------- duplicates in prv -------------- */

static const long th_prv_flags[CH_MAX] = {
	/* Due muxes we need to skip duplicated nulls */
	[CH_TASKID]    = PRV_SKIPDUP,
	[CH_TYPE]      = PRV_SKIPDUP,
	[CH_APPID]     = PRV_SKIPDUP,
	[CH_SUBSYSTEM] = PRV_SKIPDUP,
	[CH_RANK]      = PRV_SKIPDUP,
};

static const long cpu_prv_flags[CH_MAX] = {
	[CH_TASKID]    = PRV_SKIPDUP,
	[CH_TYPE]      = PRV_SKIPDUP,
	[CH_APPID]     = PRV_SKIPDUP,
	[CH_SUBSYSTEM] = PRV_SKIPDUP,
	[CH_RANK]      = PRV_SKIPDUP,
};

static const struct model_pvt_spec th_pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
	.flags = th_prv_flags,
};

static const struct model_pvt_spec cpu_pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
	.flags = cpu_prv_flags,
};

/* ----------------- tracking ------------------ */

static const int th_track[CH_MAX] = {
	[CH_TASKID]    = TRACK_TH_RUN,
	[CH_TYPE]      = TRACK_TH_RUN,
	[CH_APPID]     = TRACK_TH_RUN,
	[CH_SUBSYSTEM] = TRACK_TH_ACT,
	[CH_RANK]      = TRACK_TH_RUN,
};

static const int cpu_track[CH_MAX] = {
	[CH_TASKID]    = TRACK_TH_RUN,
	[CH_TYPE]      = TRACK_TH_RUN,
	[CH_APPID]     = TRACK_TH_RUN,
	[CH_SUBSYSTEM] = TRACK_TH_RUN,
	[CH_RANK]      = TRACK_TH_RUN,
};

/* ----------------- chan_spec ------------------ */

static const struct model_chan_spec th_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.ch_dup = chan_dup,
	.pvt = &th_pvt_spec,
	.track = th_track,
};

static const struct model_chan_spec cpu_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.pvt = &cpu_pvt_spec,
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
nosv_probe(struct emu *emu)
{
	if (emu->system.nthreads == 0)
		return 1;

	return 0;
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
nosv_create(struct emu *emu)
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
		struct nosv_thread *th = EXT(t, model_id);
		th->task_stack.thread = t;
	}

	for (struct proc *p = sys->procs; p; p = p->gnext) {
		if (init_proc(p) != 0) {
			err("init_proc failed");
			return -1;
		}
	}

	return 0;
}

int
nosv_connect(struct emu *emu)
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

			err("thread %d ended with %d stacked nosv subsystems\n",
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
	long typeid = pvt_type[CH_TYPE];
	struct pcf_type *pcftype = pcf_find_type(pcf, typeid);

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
nosv_finish(struct emu *emu)
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

	/* When running in linter mode perform additional checks */
	if (emu->args.linter_mode && end_lint(emu) != 0) {
		err("end_lint failed");
		return -1;
	}

	return 0;
}
