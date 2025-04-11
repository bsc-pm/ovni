/* Copyright (c) 2024-2025 Barcelona Supercomputing Center (BSC)
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
#include "openmp_priv.h"
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

enum {
	MUX0_LABEL = 0,
	MUX0_SUBSYSTEM = 1,
};

static int
create_cpu(struct bay *bay, struct breakdown_cpu *bcpu, int64_t gindex)
{
	enum chan_type t = CHAN_SINGLE;
	chan_init(&bcpu->out, t, "openmp.cpu%"PRIi64".breakdown.out", gindex);

	if (bay_register(bay, &bcpu->out) != 0) {
		err("bay_register out failed");
		return -1;
	}

	return 0;
}

int
model_openmp_breakdown_create(struct emu *emu)
{
	if (emu->args.breakdown == 0)
		return 0;

	struct openmp_emu *memu = EXT(emu, 'P');
	struct breakdown_emu *bemu = &memu->breakdown;

	/* Count phy cpus */
	struct system *sys = &emu->system;
	int64_t nphycpus = (int64_t) (sys->ncpus - sys->nlooms);
	bemu->nphycpus = nphycpus;

	/* Create a new Paraver trace */
	struct recorder *rec = &emu->recorder;
	bemu->pvt = recorder_add_pvt(rec, "openmp-breakdown", (long) nphycpus);
	if (bemu->pvt == NULL) {
		err("recorder_add_pvt failed");
		return -1;
	}

	if (sort_init(&bemu->sort, &emu->bay, nphycpus, "openmp.breakdown.sort") != 0) {
		err("sort_init failed");
		return -1;
	}

	for (struct cpu *cpu = sys->cpus; cpu; cpu = cpu->next) {
		if (cpu->is_virtual)
			continue;

		struct openmp_cpu *mcpu = EXT(cpu, 'P');
		struct breakdown_cpu *bcpu = &mcpu->breakdown;

		if (create_cpu(&emu->bay, bcpu, cpu->gindex) != 0) {
			err("create_cpu failed");
			return -1;
		}
	}

	return 0;
}

static int
select_mux0(struct mux *mux, struct value value, struct mux_input **input)
{
	if (value.type != VALUE_NULL)
		*input = mux_get_input(mux, MUX0_LABEL); /* label */
	else
		*input = mux_get_input(mux, MUX0_SUBSYSTEM); /* subsystem */

	return 0;
}

static int
connect_cpu(struct bay *bay, struct openmp_cpu *mcpu)
{
	struct breakdown_cpu *bcpu = &mcpu->breakdown;

	/* Channel aliases */
	struct chan *subsystem = &mcpu->m.track[CH_SUBSYSTEM].ch;
	struct chan *label = &mcpu->m.track[CH_LABEL].ch;
	struct chan *out = &bcpu->out;

	if (mux_init(&bcpu->mux0, bay, label, out, select_mux0, 2) != 0) {
		err("mux_init failed for mux0");
		return -1;
	}

	if (mux_set_input(&bcpu->mux0, MUX0_LABEL, label) != 0) {
		err("mux_set_input subsystem failed");
		return -1;
	}

	if (mux_set_input(&bcpu->mux0, MUX0_SUBSYSTEM, subsystem) != 0) {
		err("mux_set_input label failed");
		return -1;
	}

	return 0;
}

int
model_openmp_breakdown_connect(struct emu *emu)
{
	if (emu->args.breakdown == 0)
		return 0;

	struct openmp_emu *memu = EXT(emu, 'P');
	struct breakdown_emu *bemu = &memu->breakdown;
	struct bay *bay = &emu->bay;
	struct system *sys = &emu->system;

	int64_t i = 0;
	for (struct cpu *cpu = sys->cpus; cpu; cpu = cpu->next) {
		if (cpu->is_virtual)
			continue;

		struct openmp_cpu *mcpu = EXT(cpu, 'P');
		struct breakdown_cpu *bcpu = &mcpu->breakdown;

		/* Connect tri channels and muxes */
		if (connect_cpu(bay, mcpu) != 0) {
			err("connect_cpu failed");
			return -1;
		}

		/* Connect out to sort */
		if (sort_set_input(&bemu->sort, i, &bcpu->out) != 0) {
			err("sort_set_input failed");
			return -1;
		}

		/* Connect out to PRV */
		struct prv *prv = pvt_get_prv(bemu->pvt);
		long type = PRV_OPENMP_BREAKDOWN;
		long flags = PRV_SKIPDUP;

		/* We may emit zero at the start, when an input changes and all
		 * the other sort output channels write a zero in the output,
		 * before the last value is set in prv.c. */
		flags |= PRV_ZERO;

		struct chan *out = sort_get_output(&bemu->sort, i);
		if (prv_register(prv, (long) i, type, bay, out, flags)) {
			err("prv_register failed");
			return -1;
		}

		i++;
	}

	return 0;
}

int
model_openmp_breakdown_finish(struct emu *emu,
		const struct pcf_value_label **labels)
{
	if (emu->args.breakdown == 0)
		return 0;

	struct openmp_emu *memu = EXT(emu, 'P');
	struct breakdown_emu *bemu = &memu->breakdown;
	struct pcf *pcf = pvt_get_pcf(bemu->pvt);
	long typeid = PRV_OPENMP_BREAKDOWN;
	char label[] = "CPU: OpenMP Runtime/Label breakdown";
	struct pcf_type *pcftype = pcf_add_type(pcf, (int) typeid, label);
	const struct pcf_value_label *v = NULL;

	/* Emit subsystem values */
	for (v = labels[CH_SUBSYSTEM]; v->label; v++) {
		if (pcf_add_value(pcftype, v->value, v->label) == NULL) {
			err("pcf_add_value ss failed");
			return -1;
		}
	}

	/* Emit label values */
	struct system *sys = &emu->system;
	for (struct proc *p = sys->procs; p; p = p->gnext) {
		struct openmp_proc *proc = EXT(p, 'P');
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
		if (snprintf(name, 128, "~CPU %4" PRIi64, bemu->nphycpus - row) >= 128) {
			err("label too long");
			return -1;
		}

		if (prf_add(prf, (long) row, name) != 0) {
			err("prf_add failed for %s", name);
			return -1;
		}
	}

	return 0;
}
