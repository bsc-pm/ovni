#include "nodes_priv.h"

/* TODO: Assign types on runtime and generate configs */

static const int pvt_type[] = {
	[CH_SUBSYSTEM] = 30,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_SUBSYSTEM]   = "NODES subsystem",
};

static const char *pcf_suffix[TRACK_TH_MAX] = {
	[TRACK_TH_ANY] = "",
	[TRACK_TH_RUN] = "of the RUNNING thread",
	[TRACK_TH_ACT] = "of the ACTIVE thread",
};

static const struct pcf_value_label nodes_ss_values[] = {
	{ ST_REGISTER,    "Dependencies: Registering task accesses" },
	{ ST_UNREGISTER,  "Dependencies: Unregistering task accesses" },
	{ ST_IF0_WAIT,    "If0: Waiting for an If0 task" },
	{ ST_IF0_INLINE,  "If0: Executing an If0 task inline" },
	{ ST_TASKWAIT,    "Taskwait: Taskwait" },
	{ ST_CREATE,      "Add Task: Creating a task" },
	{ ST_SUBMIT,      "Add Task: Submitting a task" },
	{ ST_SPAWN,       "Spawn Function: Spawning a function" },
	{ -1, NULL },
};

static const struct pcf_value_label (*pcf_chan_value_labels[CH_MAX])[] = {
	[CH_SUBSYSTEM] = &nodes_ss_values,
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
create_type(struct pcf *pcf, enum nodes_chan c, enum nodes_chan_type ct)
{
	long type = pvt_type[c];

	if (type == -1)
		return 0;

	/* Compute the label by joining the two parts */
	const char *prefix = pcf_prefix[c];
	int track_mode = nodes_get_track(c, ct);
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
init_pcf(struct pcf *pcf, enum nodes_chan_type ct)
{
	/* Create default types and values */
	for (enum nodes_chan c = 0; c < CH_MAX; c++) {
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
	struct nodes_thread *th = EXT(thread, 'D');
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
	struct nodes_cpu *cpu = EXT(scpu, 'D');
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
nodes_init_pvt(struct emu *emu)
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
nodes_finish_pvt(struct emu *emu)
{
	UNUSED(emu);
	return 0;
}

const char *
nodes_ss_name(int ss)
{
	static const char *unknown = "(unknown)";
	const char *name = unknown;
	const struct pcf_value_label *pv;
	for (pv = &nodes_ss_values[0]; pv->label; pv++) {
		if (pv->value == ss) {
			name = pv->label;
			break;
		}
	}

	return name;
}
