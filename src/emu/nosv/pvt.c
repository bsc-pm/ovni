#include "nosv_priv.h"

/* TODO: Assign types on runtime and generate configs */

static const char *pvt_name[CT_MAX] = {
	[CT_TH]  = "thread",
	[CT_CPU] = "cpu",
};

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


static const char *pcf_suffix[TRACK_MAX] = {
	[NONE] = "",
	[RUN_TH] = "of the RUNNING thread",
	[ACT_TH] = "of the ACTIVE thread",
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

static const struct pcf_value_label (*pcf_chan_value_labels[CH_MAX])[] = {
	[CH_SUBSYSTEM] = &nosv_ss_values,
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
create_type(struct pcf *pcf, enum nosv_chan c, enum nosv_chan_type ct)
{
	long type = pvt_type[c];

	if (type == -1)
		return 0;

	/* Compute the label by joining the two parts */
	const char *prefix = pcf_prefix[c];
	int track_mode = nosv_chan_track[c][ct];
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
init_pcf(struct pcf *pcf, enum nosv_chan_type ct)
{
	/* Create default types and values */
	for (enum nosv_chan c = 0; c < CH_MAX; c++) {
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
	struct nosv_thread *th = EXT(thread, 'V');
	for (int i = 0; i < CH_MAX; i++) {
		struct chan *out = th->ch_out[i];
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
	struct nosv_cpu *cpu = EXT(scpu, 'V');
	for (int i = 0; i < CH_MAX; i++) {
		struct chan *out = &cpu->ch[i];
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
nosv_init_pvt(struct emu *emu)
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
nosv_finish_pvt(struct emu *emu)
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
			struct nosv_proc *nosvproc = EXT(p, 'V');
			struct task_info *info = &nosvproc->task_info;
			if (task_create_pcf_types(pcftype, info->types) != 0) {
				err("task_create_pcf_types failed");
				return -1;
			}
		}
	}

	return 0;
}

const char *
nosv_ss_name(int ss)
{
	static const char *unknown = "(unknown)";
	const char *name = unknown;
	const struct pcf_value_label *pv;
	for (pv = &nosv_ss_values[0]; pv->label; pv++) {
		if (pv->value == ss) {
			name = pv->label;
			break;
		}
	}

	return name;
}
