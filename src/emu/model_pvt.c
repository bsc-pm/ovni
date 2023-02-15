#include "model_pvt.h"

static const char *pcf_suffix[TRACK_TH_MAX] = {
	[TRACK_TH_ANY] = "",
	[TRACK_TH_RUN] = "of the RUNNING thread",
	[TRACK_TH_ACT] = "of the ACTIVE thread",
};

static int
create_values(const struct model_pvt_spec *pvt,
		struct pcf_type *t, int i)
{
	const struct pcf_value_label(*q)[] = pvt->label[i];

	if (q == NULL)
		return 0;

	for (const struct pcf_value_label *p = *q; p->label != NULL; p++)
		pcf_add_value(t, p->value, p->label);

	return 0;
}

static int
create_type(const struct model_pvt_spec *pvt,
		struct pcf *pcf, int i, int track_mode)
{
	long type = pvt->type[i];

	if (type == -1)
		return 0;

	/* Compute the label by joining the two parts */
	const char *prefix = pvt->prefix[i];
	const char *suffix = pcf_suffix[track_mode];

	char label[MAX_PCF_LABEL];
	int ret = snprintf(label, MAX_PCF_LABEL, "%s %s",
			prefix, suffix);

	if (ret >= MAX_PCF_LABEL) {
		err("computed type label too long");
		return -1;
	}

	struct pcf_type *pcftype = pcf_add_type(pcf, type, label);

	if (create_values(pvt, pcftype, i) != 0) {
		err("create_values failed");
		return -1;
	}

	return 0;
}

static int
init_pcf(const struct model_chan_spec *chan, struct pcf *pcf)
{
	const struct model_pvt_spec *pvt = chan->pvt;

	/* Create default types and values */
	for (int i = 0; i < chan->nch; i++) {
		int track_mode = chan->track[i];
		if (create_type(pvt, pcf, i, track_mode) != 0) {
			err("create_type failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_cpu_prv(struct emu *emu, struct cpu *scpu, struct prv *prv, int id)
{
	struct model_cpu *cpu = EXT(scpu, id);
	const struct model_chan_spec *spec = cpu->spec->chan;
	for (int i = 0; i < spec->nch; i++) {
		struct chan *out = track_get_default(&cpu->track[i]);
		long type = spec->pvt->type[i];
		long row = scpu->gindex;
		if (prv_register(prv, row, type, &emu->bay, out, PRV_DUP)) {
			err("prv_register failed");
			return -1;
		}
	}

	return 0;
}

int
model_pvt_connect_cpu(struct emu *emu, const struct model_cpu_spec *spec)
{
	struct system *sys = &emu->system;
	int id = spec->model->model;

	/* Get cpu PRV */
	struct pvt *pvt = recorder_find_pvt(&emu->recorder, "cpu");
	if (pvt == NULL) {
		err("cannot find cpu pvt");
		return -1;
	}

	/* Connect CPU channels to PRV */
	struct prv *prv = pvt_get_prv(pvt);
	for (struct cpu *c = sys->cpus; c; c = c->next) {
		if (connect_cpu_prv(emu, c, prv, id) != 0) {
			err("connect_cpu_prv failed");
			return -1;
		}
	}

	/* Init CPU PCF */
	struct pcf *pcf = pvt_get_pcf(pvt);
	if (init_pcf(spec->chan, pcf) != 0) {
		err("init_pcf failed");
		return -1;
	}

	return 0;
}

static int
connect_thread_prv(struct emu *emu, struct thread *sth, struct prv *prv, int id)
{
	struct model_thread *th = EXT(sth, id);
	const struct model_chan_spec *spec = th->spec->chan;
	for (int i = 0; i < spec->nch; i++) {
		struct chan *out = track_get_default(&th->track[i]);
		long type = spec->pvt->type[i];
		long row = sth->gindex;
		if (prv_register(prv, row, type, &emu->bay, out, PRV_DUP)) {
			err("prv_register failed");
			return -1;
		}
	}

	return 0;
}

int
model_pvt_connect_thread(struct emu *emu, const struct model_thread_spec *spec)
{
	struct system *sys = &emu->system;
	int id = spec->model->model;

	/* Get cpu PRV */
	struct pvt *pvt = recorder_find_pvt(&emu->recorder, "thread");
	if (pvt == NULL) {
		err("cannot find cpu pvt");
		return -1;
	}

	/* Connect thread channels to PRV */
	struct prv *prv = pvt_get_prv(pvt);
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		if (connect_thread_prv(emu, t, prv, id) != 0) {
			err("connect_thread_prv failed");
			return -1;
		}
	}

	/* Init thread PCF */
	struct pcf *pcf = pvt_get_pcf(pvt);
	if (init_pcf(spec->chan, pcf) != 0) {
		err("init_pcf failed");
		return -1;
	}

	return 0;
}
