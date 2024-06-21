/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "track.h"
#include <stdarg.h>
#include <stdio.h>
#include "bay.h"
#include "thread.h"

static const char *th_suffix[TRACK_TH_MAX] = {
	[TRACK_TH_ANY] = ".any",
	[TRACK_TH_RUN] = ".run",
	[TRACK_TH_ACT] = ".act",
};

static const char **track_suffix[TRACK_TYPE_MAX] = {
	[TRACK_TYPE_TH] = th_suffix,
};

int
track_init(struct track *track, struct bay *bay, enum track_type type, int mode, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	int n = ARRAYLEN(track->name);
	int ret = vsnprintf(track->name, n, fmt, ap);
	if (ret >= n) {
		err("track name too long");
		return -1;
	}
	va_end(ap);

	track->type = type;
	track->mode = mode;
	track->bay = bay;

	/* Set the channel name by appending the suffix */
	struct chan *ch = &track->ch;
	const char *suffix = track_suffix[type][mode];
	chan_init(ch, CHAN_SINGLE, "%s%s", track->name, suffix);

	if (bay_register(bay, ch) != 0) {
		err("bay_register failed");
		return -1;
	}

	return 0;
}

struct chan *
track_get_output(struct track *track)
{
	return track->out;
}

int
track_set_select(struct track *track, struct chan *sel, mux_select_func_t fsel, int64_t ninputs)
{
	struct mux *mux = &track->mux;
	struct chan *out = &track->ch;
	if (mux_init(mux, track->bay, sel, out, fsel, ninputs) != 0) {
		err("mux_init failed");
		return -1;
	}
	track->out = out;

	return 0;
}

int
track_set_input(struct track *track, int64_t index, struct chan *inp)
{
	struct mux *mux = &track->mux;
	if (mux_set_input(mux, index, inp) != 0) {
		err("mux_add_input failed");
		return -1;
	}

	return 0;
}

static int
track_th_input_chan(struct track *track, struct chan *sel, struct chan *inp)
{
	mux_select_func_t fsel = NULL;

	if (track->mode == TRACK_TH_ANY) {
		/* Just follow the channel as-is */
		track->out = inp;
		return 0;
	}

	/* Otherwise create a mux and use a selection function */
	switch (track->mode) {
		case TRACK_TH_RUN:
			fsel = thread_select_running;
			break;
		case TRACK_TH_ACT:
			fsel = thread_select_active;
			break;
		default:
			err("wrong mode");
			return -1;
	};

	/* Create all thread tracking modes */
	if (track_set_select(track, sel, fsel, 1) != 0) {
		err("track_select failed");
		return -1;
	}

	if (track_set_input(track, 0, inp) != 0) {
		err("track_add_input failed");
		return -1;
	}

	return 0;
}

int
track_connect_thread(struct track *tracks, struct chan *chans, struct chan *sel, int n)
{
	for (int i = 0; i < n; i++) {
		struct track *track = &tracks[i];

		if (track_th_input_chan(track, sel, &chans[i]) != 0) {
			err("track_th_input_chan failed");
			return -1;
		}
	}

	return 0;
}
