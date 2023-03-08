/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

//#define ENABLE_DEBUG

#include "nanos6_priv.h"

#include "emu_prv.h"

static int
create_cpu(struct bay *bay, struct breakdown_cpu *bcpu, int64_t gindex)
{
	enum chan_type t = CHAN_SINGLE;
	chan_init(&bcpu->tr,  t, "nanos6.cpu%ld.breakdown.tr",  gindex);
	chan_init(&bcpu->tri, t, "nanos6.cpu%ld.breakdown.tri", gindex);

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
model_nanos6_breakdown_create(struct emu *emu)
{
	if (emu->args.breakdown == 0)
		return 0;

	struct nanos6_emu *memu = EXT(emu, '6');
	struct breakdown_emu *bemu = &memu->brk;

	/* Count phy cpus */
	struct system *sys = &emu->system;
	int nphycpus = sys->ncpus - sys->nlooms;

	/* Create a new Paraver trace */
	struct recorder *rec = &emu->recorder;
	bemu->pvt = recorder_add_pvt(rec, "nanos6-breakdown", nphycpus);
	if (bemu->pvt == NULL) {
		err("recorder_add_pvt failed");
		return -1;
	}

	if (sort_init(&bemu->sort, &emu->bay, nphycpus, "nanos6.breakdown.sort") != 0) {
		err("sort_init failed");
		return -1;
	}

	int64_t i = 0;
	for (struct cpu *cpu = sys->cpus; cpu; cpu = cpu->next) {
		if (cpu->is_virtual)
			continue;

		struct nanos6_cpu *mcpu = EXT(cpu, '6');
		struct breakdown_cpu *bcpu = &mcpu->brk;

		if (create_cpu(&emu->bay, bcpu, cpu->gindex) != 0) {
			err("create_cpu failed");
			return -1;
		}

		i++;
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
	
	int64_t i = in_body;
	char *inputs[] = { "subsystem", "task_type" };
	dbg("selecting input %ld (%s)", i, inputs[i]);
	*input = mux_get_input(mux, i);

	return 0;
}

static int
select_idle(struct mux *mux, struct value value, struct mux_input **input)
{
	char buf[128];
	dbg("value is %s", value_str(value, buf));
	if (value.type == VALUE_INT64 && value.i == ST_WORKER_IDLE) {
		dbg("selecting input 1 (idle)");
		*input = mux_get_input(mux, 1);
	} else {
		dbg("selecting input 0 (tr)");
		*input = mux_get_input(mux, 0);
	}

	return 0;
}

static int
connect_cpu(struct bay *bay, struct nanos6_cpu *mcpu)
{
	struct breakdown_cpu *bcpu = &mcpu->brk;

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
model_nanos6_breakdown_connect(struct emu *emu)
{
	if (emu->args.breakdown == 0)
		return 0;

	struct nanos6_emu *memu = EXT(emu, '6');
	struct breakdown_emu *bemu = &memu->brk;
	struct bay *bay = &emu->bay;
	struct system *sys = &emu->system;

	int64_t i = 0;
	for (struct cpu *cpu = sys->cpus; cpu; cpu = cpu->next) {
		if (cpu->is_virtual)
			continue;

		struct nanos6_cpu *mcpu = EXT(cpu, '6');
		struct breakdown_cpu *bcpu = &mcpu->brk;

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
		long type = PRV_NANOS6_BREAKDOWN;
		long flags = PRV_SKIPDUP;

		struct chan *out = sort_get_output(&bemu->sort, i);
		if (prv_register(prv, i, type, bay, out, flags)) {
			err("prv_register failed");
			return -1;
		}

		i++;
	}

	return 0;
}