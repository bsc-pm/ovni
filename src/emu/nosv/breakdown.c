/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "breakdown.h"
#include <stdint.h>
#include <stdio.h>
#include "bay.h"
#include "chan.h"
#include "common.h"
#include "cpu.h"
#include "emu.h"
#include "emu_args.h"
#include "emu_prv.h"
#include "extend.h"
#include "model_cpu.h"
#include "mux.h"
#include "nosv_priv.h"
#include "proc.h"
#include "pv/pcf.h"
#include "pv/prf.h"
#include "pv/prv.h"
#include "pv/pvt.h"
#include "recorder.h"
#include "sort.h"
#include "system.h"
#include "task.h"
#include "track.h"
#include "value.h"


static int
create_cpu(struct bay *bay, struct nosv_breakdown_cpu *bcpu, int64_t gindex)
{
	enum chan_type t = CHAN_SINGLE;
	chan_init(&bcpu->tr,  t, "nosv.cpu%ld.breakdown.tr",  gindex);
	chan_init(&bcpu->tri, t, "nosv.cpu%ld.breakdown.tri", gindex);

	/* Register all channels in the bay */
	if (bay_register(bay, &bcpu->tr) != 0) {
		err("bay_register tr failed");
		return -1;
	}
	if (bay_register(bay, &bcpu->tri) != 0) {
		err("bay_register tri failed");
		return -1;
	}

	return 0;
}

int
model_nosv_breakdown_create(struct emu *emu)
{
	if (emu->args.breakdown == 0)
		return 0;

	struct nosv_emu *memu = EXT(emu, 'V');
	struct nosv_breakdown_emu *bemu = &memu->breakdown;

	/* Count phy cpus */
	struct system *sys = &emu->system;
	int64_t nphycpus = sys->ncpus - sys->nlooms;
	bemu->nphycpus = nphycpus;

	/* Create a new Paraver trace */
	struct recorder *rec = &emu->recorder;
	bemu->pvt = recorder_add_pvt(rec, "nosv-breakdown", nphycpus);
	if (bemu->pvt == NULL) {
		err("recorder_add_pvt failed");
		return -1;
	}

	if (sort_init(&bemu->sort, &emu->bay, nphycpus, "nosv.breakdown.sort") != 0) {
		err("sort_init failed");
		return -1;
	}

	for (struct cpu *cpu = sys->cpus; cpu; cpu = cpu->next) {
		if (cpu->is_virtual)
			continue;

		struct nosv_cpu *mcpu = EXT(cpu, 'V');
		struct nosv_breakdown_cpu *bcpu = &mcpu->breakdown;

		if (create_cpu(&emu->bay, bcpu, cpu->gindex) != 0) {
			err("create_cpu failed");
			return -1;
		}
	}

	return 0;
}

static int
select_tr(struct mux *mux, struct value value, struct mux_input **input)
{
	/* Only select the task if we are in ST_TASK_BODY and the task_type has
	 * a non-null value */

	int64_t in_body = (value.type == VALUE_INT64 && value.i == ST_TASK_BODY);

	if (in_body) {
		struct value tt;
		struct mux_input *ttinput = mux_get_input(mux, 1);
		if (chan_read(ttinput->chan, &tt) != 0) {
			err("chan_read failed");
			return -1;
		}

		/* Only show task type if we have a task */
		if (tt.type == VALUE_NULL)
			in_body = 0;
	}

	if (!in_body) {
		/* Only select ss if not NULL */
		struct value ss;
		struct mux_input *ssinput = mux_get_input(mux, 0);
		if (chan_read(ssinput->chan, &ss) != 0) {
			err("chan_read failed");
			return -1;
		}

		/* Don't select anything, so the default output is shown */
		if (ss.type == VALUE_NULL) {
			dbg("not selecting anything");
			*input = NULL;
			return 0;
		}
	}

	int64_t i = in_body;
	char *inputs[] = { "subsystem", "task_type" };
	dbg("selecting input %ld (%s)", i, inputs[i]);
	*input = mux_get_input(mux, i);

	return 0;
}

static int
select_idle(struct mux *mux, struct value value, struct mux_input **input)
{
	dbg("selecting tri output for value %s", value_str(value));

	if (value.type == VALUE_INT64 && value.i == ST_PROGRESSING) {
		dbg("selecting input 0 (tr)");
		*input = mux_get_input(mux, 0);
	} else {
		dbg("selecting input 1 (idle)");
		*input = mux_get_input(mux, 1);
	}

	return 0;
}

static int
connect_cpu(struct bay *bay, struct nosv_cpu *mcpu)
{
	struct nosv_breakdown_cpu *bcpu = &mcpu->breakdown;

	/* Channel aliases */
	struct chan *ss = &mcpu->m.track[CH_SUBSYSTEM].ch;
	struct chan *tt = &mcpu->m.track[CH_TYPE].ch;
	struct chan *idle = &mcpu->m.track[CH_IDLE].ch;
	struct chan *tr = &bcpu->tr;
	struct chan *tri = &bcpu->tri;

	/* Connect mux0 using ss as select */
	if (mux_init(&bcpu->mux0, bay, ss, tr, select_tr, 2) != 0) {
		err("mux_init failed");
		return -1;
	}

	if (mux_set_input(&bcpu->mux0, 0, ss) != 0) {
		err("mux_set_input ss failed");
		return -1;
	}

	if (mux_set_input(&bcpu->mux0, 1, tt) != 0) {
		err("mux_set_input tt failed");
		return -1;
	}

	/* TODO what do we emit on null? */
	mux_set_default(&bcpu->mux0, value_int64(666));

	/* Connect mux 1 using idle as select */
	if (mux_init(&bcpu->mux1, bay, idle, tri, select_idle, 2) != 0) {
		err("mux_init failed");
		return -1;
	}

	if (mux_set_input(&bcpu->mux1, 0, tr) != 0) {
		err("mux_set_input tr failed");
		return -1;
	}

	if (mux_set_input(&bcpu->mux1, 1, idle) != 0) {
		err("mux_set_input idle failed");
		return -1;
	}

	return 0;
}

int
model_nosv_breakdown_connect(struct emu *emu)
{
	if (emu->args.breakdown == 0)
		return 0;

	struct nosv_emu *memu = EXT(emu, 'V');
	struct nosv_breakdown_emu *bemu = &memu->breakdown;
	struct bay *bay = &emu->bay;
	struct system *sys = &emu->system;

	int64_t i = 0;
	for (struct cpu *cpu = sys->cpus; cpu; cpu = cpu->next) {
		if (cpu->is_virtual)
			continue;

		struct nosv_cpu *mcpu = EXT(cpu, 'V');
		struct nosv_breakdown_cpu *bcpu = &mcpu->breakdown;

		/* Connect tr and tri channels and muxes */
		if (connect_cpu(bay, mcpu) != 0) {
			err("connect_cpu failed");
			return -1;
		}

		/* Connect tri to sort */
		if (sort_set_input(&bemu->sort, i, &bcpu->tri) != 0) {
			err("sort_set_input failed");
			return -1;
		}

		/* Connect out to PRV */
		struct prv *prv = pvt_get_prv(bemu->pvt);
		long type = PRV_NOSV_BREAKDOWN;
		long flags = PRV_SKIPDUP | PRV_ZERO;

		struct chan *out = sort_get_output(&bemu->sort, i);
		if (prv_register(prv, i, type, bay, out, flags)) {
			err("prv_register failed");
			return -1;
		}

		i++;
	}

	return 0;
}

int
model_nosv_breakdown_finish(struct emu *emu,
		const struct pcf_value_label **labels)
{
	if (emu->args.breakdown == 0)
		return 0;

	struct nosv_emu *memu = EXT(emu, 'V');
	struct nosv_breakdown_emu *bemu = &memu->breakdown;
	struct pcf *pcf = pvt_get_pcf(bemu->pvt);
	long typeid = PRV_NOSV_BREAKDOWN;
	char label[] = "CPU: nOS-V Runtime/Idle/Task breakdown";
	struct pcf_type *pcftype = pcf_add_type(pcf, typeid, label);
	const struct pcf_value_label *v = NULL;

	/* Emit subsystem values */
	for (v = labels[CH_SUBSYSTEM]; v->label; v++) {
		if (pcf_add_value(pcftype, v->value, v->label) == NULL) {
			err("pcf_add_value ss failed");
			return -1;
		}
	}

	/* Emit idle values */
	for (v = labels[CH_IDLE]; v->label; v++) {
		if (pcf_add_value(pcftype, v->value, v->label) == NULL) {
			err("pcf_add_value idle failed");
			return -1;
		}
	}

	/* Emit task_type values */
	struct system *sys = &emu->system;
	for (struct proc *p = sys->procs; p; p = p->gnext) {
		struct nosv_proc *proc = EXT(p, 'V');
		struct task_info *info = &proc->task_info;
		if (task_create_pcf_types(pcftype, info->types) != 0) {
			err("task_create_pcf_types failed");
			return -1;
		}
	}

	/* Also populate the row labels */
	struct prf *prf = pvt_get_prf(bemu->pvt);
	for (int64_t row = 0; row < bemu->nphycpus; row++) {
		char name[128];
		if (snprintf(name, 128, "~CPU %4ld", bemu->nphycpus - row) >= 128) {
			err("label too long");
			return -1;
		}

		if (prf_add(prf, row, name) != 0) {
			err("prf_add failed for %s", name);
			return -1;
		}
	}

	return 0;
}
