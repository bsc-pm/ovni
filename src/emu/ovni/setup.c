/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ovni_priv.h"
#include <stddef.h>
#include "common.h"
#include "emu.h"
#include "emu_prv.h"
#include "model.h"
#include "model_chan.h"
#include "model_cpu.h"
#include "model_pvt.h"
#include "model_thread.h"
#include "pv/pcf.h"
#include "pv/prv.h"
#include "system.h"
#include "thread.h"
#include "track.h"

static const char model_name[] = "ovni";
enum { model_id = 'O' };

struct model_spec model_ovni = {
	.name    = model_name,
	.version = "1.0.0",
	.model   = model_id,
	.create  = model_ovni_create,
	.connect = model_ovni_connect,
	.event   = model_ovni_event,
	.probe   = model_ovni_probe,
	.finish  = model_ovni_finish,
};

/* ----------------- channels ------------------ */

static const char *chan_name[CH_MAX] = {
	[CH_FLUSH] = "flush",
};

static const int chan_stack[CH_MAX] = { 0 };

/* ----------------- pvt ------------------ */

static const int pvt_type[CH_MAX] = {
	[CH_FLUSH] = PRV_OVNI_FLUSH,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_FLUSH] = "Flushing ovni buffer",
};

static const struct pcf_value_label flushing_values[] = {
	{ ST_FLUSHING, "Flushing" },
	{ -1, NULL },
};

static const struct pcf_value_label *pcf_labels[CH_MAX] = {
	[CH_FLUSH] = flushing_values,
};

static const long prv_flags[CH_MAX] = {
	[CH_FLUSH] = PRV_SKIPDUP,
};

static const struct model_pvt_spec pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
	.flags = prv_flags,
};

/* ----------------- tracking ------------------ */

static const int th_track[CH_MAX] = {
	[CH_FLUSH] = TRACK_TH_ANY,
};

static const int cpu_track[CH_MAX] = {
	[CH_FLUSH] = TRACK_TH_RUN,
};

/* ----------------- chan_spec ------------------ */

static const struct model_chan_spec th_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
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

static const struct model_cpu_spec cpu_spec = {
	.size = sizeof(struct ovni_cpu),
	.chan = &cpu_chan,
	.model = &model_ovni,
};

static const struct model_thread_spec th_spec = {
	.size = sizeof(struct ovni_thread),
	.chan = &th_chan,
	.model = &model_ovni,
};

/* ----------------------------------------------------- */

int
model_ovni_probe(struct emu *emu)
{
	return model_version_probe(&model_ovni, emu);
}

int
model_ovni_create(struct emu *emu)
{
	if (model_thread_create(emu, &th_spec) != 0) {
		err("model_thread_init failed");
		return -1;
	}

	if (model_cpu_create(emu, &cpu_spec) != 0) {
		err("model_cpu_init failed");
		return -1;
	}

	return 0;
}

int
model_ovni_connect(struct emu *emu)
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

int
model_ovni_finish(struct emu *emu)
{
	/* Skip the check if the we are stopping prematurely */
	if (!emu->finished)
		return 0;

	struct system *sys = &emu->system;
	int ret = 0;

	/* Ensure that all threads are in the Dead state */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		if (t->state != TH_ST_DEAD) {
			err("thread %d is not dead (%s)", t->tid, t->id);
			ret = -1;
		}
	}

	return ret;
}
