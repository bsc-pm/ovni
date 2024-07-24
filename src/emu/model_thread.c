/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "model_thread.h"
#include <stdint.h>
#include <stdlib.h>
#include "bay.h"
#include "chan.h"
#include "common.h"
#include "emu.h"
#include "extend.h"
#include "model.h"
#include "model_chan.h"
#include "model_pvt.h"
#include "system.h"
#include "thread.h"
#include "track.h"

static int
init_chan(struct model_thread *th, const struct model_chan_spec *spec, int64_t gindex)
{
	const char *fmt = "%s.thread%"PRIi64".%s";
	const char *prefix = spec->prefix;

	th->ch = calloc((size_t) spec->nch, sizeof(struct chan));
	if (th->ch == NULL) {
		err("calloc failed:");
		return -1;
	}

	for (int i = 0; i < spec->nch; i++) {
		struct chan *c = &th->ch[i];
		enum chan_type type = spec->ch_stack[i] ? CHAN_STACK : CHAN_SINGLE;
		const char *ch_name = spec->ch_names[i];
		chan_init(c, type, fmt, prefix, gindex, ch_name);

		if (spec->ch_dup != NULL) {
			int dup = spec->ch_dup[i];
			chan_prop_set(c, CHAN_ALLOW_DUP, dup);
		}

		if (bay_register(th->bay, c) != 0) {
			err("bay_register failed");
			return -1;
		}
	}

	th->track = calloc((size_t) spec->nch, sizeof(struct track));
	if (th->track == NULL) {
		err("calloc failed:");
		return -1;
	}

	for (int i = 0; i < spec->nch; i++) {
		struct track *track = &th->track[i];

		const char *ch_name = spec->ch_names[i];
		int track_mode = spec->track[i];

		if (track_init(track, th->bay, TRACK_TYPE_TH,
					track_mode, fmt,
					prefix, gindex, ch_name) != 0) {
			err("track_init failed");
			return -1;
		}
	}

	return 0;
}

static int
init_thread(struct thread *systh, struct bay *bay, const struct model_thread_spec *spec)
{
	struct model_thread *th = calloc(1, spec->size);
	if (th == NULL) {
		err("calloc failed:");
		return -1;
	}

	th->spec = spec;
	th->bay = bay;

	if (init_chan(th, spec->chan, systh->gindex) != 0) {
		err("init_chan failed");
		return -1;
	}

	extend_set(&systh->ext, spec->model->model, th);

	return 0;
}

int
model_thread_create(struct emu *emu, const struct model_thread_spec *spec)
{
	struct system *sys = &emu->system;
	struct bay *bay = &emu->bay;

	for (struct thread *t = sys->threads; t; t = t->gnext) {
		if (init_thread(t, bay, spec) != 0) {
			err("init_thread failed");
			return -1;
		}
	}

	return 0;
}

int
model_thread_connect(struct emu *emu, const struct model_thread_spec *spec)
{
	int id = spec->model->model;
	struct system *sys = &emu->system;

	for (struct thread *t = sys->threads; t; t = t->gnext) {
		struct model_thread *th = EXT(t, id);
		struct chan *sel = &t->chan[TH_CHAN_STATE];
		int nch = th->spec->chan->nch;
		if (track_connect_thread(th->track, th->ch, sel, nch) != 0) {
			err("track_thread failed");
			return -1;
		}
	}

	/* Connect channels to Paraver trace */
	if (model_pvt_connect_thread(emu, spec) != 0) {
		err("model_pvt_connect_thread failed");
		return -1;
	}

	return 0;
}
