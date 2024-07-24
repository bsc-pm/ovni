/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "model_cpu.h"
#include <stdint.h>
#include <stdlib.h>
#include "chan.h"
#include "common.h"
#include "cpu.h"
#include "emu.h"
#include "extend.h"
#include "model.h"
#include "model_chan.h"
#include "model_pvt.h"
#include "model_thread.h"
#include "system.h"
#include "thread.h"
#include "track.h"
struct bay;

static struct model_cpu *
get_model_cpu(struct cpu *cpu, int id)
{
	return EXT(cpu, id);
}

static int
init_chan(struct model_cpu *cpu, const struct model_chan_spec *spec, int64_t gindex)
{
	cpu->track = calloc((size_t) spec->nch, sizeof(struct track));
	if (cpu->track == NULL) {
		err("calloc failed:");
		return -1;
	}

	for (int i = 0; i < spec->nch; i++) {
		struct track *track = &cpu->track[i];

		const char *name = cpu->spec->model->name;
		const char *ch_name = spec->ch_names[i];
		int track_mode = spec->track[i];

		if (track_init(track, cpu->bay, TRACK_TYPE_TH, track_mode,
					"%s.cpu%"PRIi64".%s",
					name, gindex, ch_name) != 0) {
			err("track_init failed");
			return -1;
		}
	}

	return 0;
}

static int
init_cpu(struct cpu *syscpu, struct bay *bay, const struct model_cpu_spec *spec)
{
	/* The first member must be a struct model_cpu */
	struct model_cpu *cpu = calloc(1, spec->size);
	if (cpu == NULL) {
		err("calloc failed:");
		return -1;
	}

	cpu->spec = spec;
	cpu->bay = bay;

	if (init_chan(cpu, spec->chan, syscpu->gindex) != 0) {
		err("init_chan failed");
		return -1;
	}

	extend_set(&syscpu->ext, spec->model->model, cpu);
	return 0;
}

int
model_cpu_create(struct emu *emu, const struct model_cpu_spec *spec)
{
	struct system *sys = &emu->system;
	struct bay *bay = &emu->bay;

	for (struct cpu *c = sys->cpus; c; c = c->next) {
		if (init_cpu(c, bay, spec) != 0) {
			err("init_cpu failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_cpu(struct emu *emu, struct cpu *scpu, int id)
{
	struct model_cpu *cpu = get_model_cpu(scpu, id);
	const struct model_chan_spec *chan_spec = cpu->spec->chan;

	for (int i = 0; i < chan_spec->nch; i++) {
		struct track *track = &cpu->track[i];

		/* Choose select CPU channel based on tracking mode (only
		 * TRACK_TH_RUN allowed, as active may cause collisions) */
		int mode = chan_spec->track[i];
		if (mode != TRACK_TH_RUN) {
			err("only TRACK_TH_RUN allowed");
			return -1;
		}

		struct chan *sel = cpu_get_th_chan(scpu);

		int64_t nthreads = (int64_t) emu->system.nthreads;
		if (track_set_select(track, sel, NULL, nthreads) != 0) {
			err("track_select failed");
			return -1;
		}

		/* Add each thread as input */
		for (struct thread *t = emu->system.threads; t; t = t->gnext) {
			struct model_thread *th = EXT(t, id);

			/* Use the input thread directly */
			struct chan *inp = &th->ch[i];

			if (track_set_input(track, t->gindex, inp) != 0) {
				err("track_add_input failed");
				return -1;
			}
		}
	}

	return 0;
}

int
model_cpu_connect(struct emu *emu, const struct model_cpu_spec *spec)
{
	struct system *sys = &emu->system;
	int id = spec->model->model;

	/* Connect track channels */
	for (struct cpu *c = sys->cpus; c; c = c->next) {
		if (connect_cpu(emu, c, id) != 0) {
			err("connect_cpu failed");
			return -1;
		}
	}

	/* Connect channels to Paraver trace */
	if (model_pvt_connect_cpu(emu, spec) != 0) {
		err("model_pvt_connect_cpu failed");
		return -1;
	}

	return 0;
}
