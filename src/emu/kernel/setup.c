/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "kernel_priv.h"
#include <stddef.h>
#include "common.h"
#include "emu.h"
#include "emu_prv.h"
#include "ev_spec.h"
#include "model.h"
#include "model_chan.h"
#include "model_cpu.h"
#include "model_pvt.h"
#include "model_thread.h"
#include "pv/pcf.h"
#include "pv/prv.h"
#include "system.h"
#include "track.h"

static const char model_name[] = "kernel";
enum { model_id = 'K' };

static struct ev_decl model_evlist[] = {
	{ "KO[", "out of CPU" },
	{ "KO]", "back to CPU" },
	{ NULL, NULL },
};

struct model_spec model_kernel = {
	.name    = model_name,
	.version = "1.0.0",
	.evlist  = model_evlist,
	.model   = model_id,
	.create  = model_kernel_create,
	.connect = model_kernel_connect,
	.event   = model_kernel_event,
	.probe   = model_kernel_probe,
};

/* ----------------- channels ------------------ */

static const char *chan_name[CH_MAX] = {
	[CH_CS] = "context_switch",
};

static const int chan_stack[CH_MAX] = {
	[CH_CS] = 1,
};

/* ----------------- pvt ------------------ */

static const int pvt_type[CH_MAX] = {
	[CH_CS] = PRV_KERNEL_CS,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_CS] = "Kernel context switch",
};

static const struct pcf_value_label kernel_cs_values[] = {
	{ ST_CSOUT, "Context switch: Out of the CPU" },
	{ -1, NULL },
};

static const struct pcf_value_label *pcf_labels[CH_MAX] = {
	[CH_CS] = kernel_cs_values,
};

static const long prv_flags[CH_MAX] = {
	[CH_CS] = PRV_SKIPDUP,
};

static const struct model_pvt_spec pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
	.flags = prv_flags,
};

/* ----------------- tracking ------------------ */

static const int th_track[CH_MAX] = {
	[CH_CS] = TRACK_TH_ANY,
};

static const int cpu_track[CH_MAX] = {
	[CH_CS] = TRACK_TH_RUN,
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
	.size = sizeof(struct kernel_cpu),
	.chan = &cpu_chan,
	.model = &model_kernel,
};

static const struct model_thread_spec th_spec = {
	.size = sizeof(struct kernel_thread),
	.chan = &th_chan,
	.model = &model_kernel,
};

/* ----------------------------------------------------- */

int
model_kernel_probe(struct emu *emu)
{
	return model_version_probe(&model_kernel, emu);
}

int
model_kernel_create(struct emu *emu)
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
model_kernel_connect(struct emu *emu)
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
