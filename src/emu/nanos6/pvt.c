#include "nanos6_priv.h"

/* TODO: Assign types on runtime and generate configs */

static const char *pvt_name[CT_MAX] = {
	[CT_TH]  = "thread",
	[CT_CPU] = "cpu",
};

static const int pvt_type[] = {
	[CH_TASKID]    = 35,
	[CH_TYPE]      = 36,
	[CH_SUBSYSTEM] = 37,
	[CH_RANK]      = 38,
	[CH_THREAD]    = 39,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_TASKID]    = "Nanos6 task ID",
	[CH_TYPE]      = "Nanos6 task type",
	[CH_SUBSYSTEM] = "Nanos6 subsystem",
	[CH_RANK]      = "Nanos6 task MPI rank",
	[CH_THREAD]    = "Nanos6 thread type",
};

static const char *pcf_suffix[TRACK_TH_MAX] = {
	[TRACK_TH_ANY] = "",
	[TRACK_TH_RUN] = "of the RUNNING thread",
	[TRACK_TH_ACT] = "of the ACTIVE thread",
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
	{ EV_SIGNAL,           "EV Worker: Wakening another thread" },
	{ -1, NULL },
};

static const struct pcf_value_label nanos6_thread_type[] = {
	{ ST_TH_EXTERNAL,   "External" },
	{ ST_TH_WORKER,     "Worker" },
	{ ST_TH_LEADER,     "Leader" },
	{ ST_TH_MAIN,       "Main" },
	{ -1, NULL },
};

static const struct pcf_value_label (*pcf_chan_value_labels[CH_MAX])[] = {
	[CH_SUBSYSTEM] = &nanos6_ss_values,
	[CH_THREAD]    = &nanos6_thread_type,
};

/* ------------------------------ pcf ------------------------------ */

static int
create_values(struct pcf_type *t, int c)
{
	const struct pcf_value_label(*q)[] = pcf_chan_value_labels[c];

	if (q == NULL)
		return 0;

	for (const struct pcf_value_label *p = *q; p->label != NULL; p++)
		pcf_add_value(t, p->value, p->label);

	return 0;
}

static int
create_type(struct pcf *pcf, enum nanos6_chan c, enum nanos6_chan_type ct)
{
	long type = pvt_type[c];

	if (type == -1)
		return 0;

	/* Compute the label by joining the two parts */
	const char *prefix = pcf_prefix[c];
	int track_mode = nanos6_chan_track[c][ct];
	const char *suffix = pcf_suffix[track_mode];

	char label[MAX_PCF_LABEL];
	int ret = snprintf(label, MAX_PCF_LABEL, "%s %s",
			prefix, suffix);

	if (ret >= MAX_PCF_LABEL) {
		err("computed type label too long");
		return -1;
	}

	struct pcf_type *pcftype = pcf_add_type(pcf, type, label);

	return create_values(pcftype, c);
}

static int
init_pcf(struct pcf *pcf, enum nanos6_chan_type ct)
{
	/* Create default types and values */
	for (enum nanos6_chan c = 0; c < CH_MAX; c++) {
		if (create_type(pcf, c, ct) != 0) {
			err("create_type failed");
			return -1;
		}
	}

	return 0;
}

/* ------------------------------ prv ------------------------------ */

static int
connect_thread_prv(struct emu *emu, struct thread *thread, struct prv *prv)
{
	struct nanos6_thread *th = EXT(thread, '6');
	for (int i = 0; i < CH_MAX; i++) {
		struct chan *out = track_get_default(&th->track[i]);
		long type = pvt_type[i];
		long row = thread->gindex;
		if (prv_register(prv, row, type, &emu->bay, out, PRV_DUP)) {
			err("prv_register failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_cpu_prv(struct emu *emu, struct cpu *scpu, struct prv *prv)
{
	struct nanos6_cpu *cpu = EXT(scpu, '6');
	for (int i = 0; i < CH_MAX; i++) {
		struct chan *out = track_get_default(&cpu->track[i]);
		long type = pvt_type[i];
		long row = scpu->gindex;
		if (prv_register(prv, row, type, &emu->bay, out, PRV_DUP)) {
			err("prv_register failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_threads(struct emu *emu)
{
	struct system *sys = &emu->system;

	/* Get thread PRV */
	struct pvt *pvt = recorder_find_pvt(&emu->recorder, "thread");
	if (pvt == NULL) {
		err("cannot find thread pvt");
		return -1;
	}

	/* Connect thread channels to PRV */
	struct prv *prv = pvt_get_prv(pvt);
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		if (connect_thread_prv(emu, t, prv) != 0) {
			err("connect_thread_prv failed");
			return -1;
		}
	}

	/* Init thread PCF */
	struct pcf *pcf = pvt_get_pcf(pvt);
	if (init_pcf(pcf, CT_TH) != 0) {
		err("init_pcf failed");
		return -1;
	}

	return 0;
}

static int
connect_cpus(struct emu *emu)
{
	struct system *sys = &emu->system;

	/* Get cpu PRV */
	struct pvt *pvt = recorder_find_pvt(&emu->recorder, "cpu");
	if (pvt == NULL) {
		err("cannot find cpu pvt");
		return -1;
	}

	/* Connect CPU channels to PRV */
	struct prv *prv = pvt_get_prv(pvt);
	for (struct cpu *c = sys->cpus; c; c = c->next) {
		if (connect_cpu_prv(emu, c, prv) != 0) {
			err("connect_cpu_prv failed");
			return -1;
		}
	}

	/* Init CPU PCF */
	struct pcf *pcf = pvt_get_pcf(pvt);
	if (init_pcf(pcf, CT_CPU) != 0) {
		err("init_pcf failed");
		return -1;
	}

	return 0;
}

/* Connect all outputs to the paraver trace and setup PCF types */
int
nanos6_init_pvt(struct emu *emu)
{
	if (connect_threads(emu) != 0) {
		err("connect_threads failed");
		return -1;
	}

	if (connect_cpus(emu) != 0) {
		err("connect_cpus failed");
		return -1;
	}

	return 0;
}

int
nanos6_finish_pvt(struct emu *emu)
{
	struct system *sys = &emu->system;

	/* Emit task types for all channel types and processes */
	for (enum chan_type ct = 0; ct < CHAN_MAXTYPE; ct++) {
		struct pvt *pvt = recorder_find_pvt(&emu->recorder, pvt_name[ct]);
		if (pvt == NULL) {
			err("cannot find pvt with name '%s'", pvt_name[ct]);
			return -1;
		}
		struct pcf *pcf = pvt_get_pcf(pvt);
		long typeid = pvt_type[CH_TYPE];
		struct pcf_type *pcftype = pcf_find_type(pcf, typeid);

		for (struct proc *p = sys->procs; p; p = p->gnext) {
			struct nanos6_proc *nanos6proc = EXT(p, '6');
			struct task_info *info = &nanos6proc->task_info;
			if (task_create_pcf_types(pcftype, info->types) != 0) {
				err("task_create_pcf_types failed");
				return -1;
			}
		}
	}

	return 0;
}

const char *
nanos6_ss_name(int ss)
{
	static const char *unknown = "(unknown)";
	const char *name = unknown;
	const struct pcf_value_label *pv;
	for (pv = &nanos6_ss_values[0]; pv->label; pv++) {
		if (pv->value == ss) {
			name = pv->label;
			break;
		}
	}

	return name;
}
