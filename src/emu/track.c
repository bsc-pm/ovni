/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "track.h"

#include "thread.h"

static const int track_nmodes[TRACK_TYPE_MAX] = {
	[TRACK_TYPE_TH] = TRACK_TH_MAX,
};

static const char *th_suffix[TRACK_TH_MAX] = {
	[TRACK_TH_ANY] = ".any",
	[TRACK_TH_RUN] = ".run",
	[TRACK_TH_ACT] = ".act",
};

static const char **track_suffix[TRACK_TYPE_MAX] = {
	[TRACK_TYPE_TH] = th_suffix,
};

int
track_init(struct track *track, struct bay *bay, enum track_type type, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	int n = ARRAYLEN(track->name);
	int ret = vsnprintf(track->name, n, fmt, ap);
	if (ret >= n)
		die("track name too long\n");
	va_end(ap);

	track->nmodes = track_nmodes[type];
	track->type = type;
	track->bay = bay;

	/* Create one output channel per tracking mode */
	track->out = calloc(track->nmodes, sizeof(struct chan));
	if (track->out == NULL) {
		err("calloc failed:");
		return -1;
	}

	/* Set the channel name by appending the suffix */
	for (int i = 0; i < track->nmodes; i++) {
		struct chan *ch = &track->out[i];
		chan_init(ch, CHAN_SINGLE, "%s%s", track->name, track_suffix[type][i]);

		if (bay_register(bay, ch) != 0) {
			err("bay_register failed");
			return -1;
		}
	}

	track->mux = calloc(track->nmodes, sizeof(struct mux));
	if (track->mux == NULL) {
		err("calloc failed:");
		return -1;
	}

	return 0;
}

void
track_set_default(struct track *track, int mode)
{
	track->def = &track->out[mode];
}

struct chan *
track_get_default(struct track *track)
{
	return track->def;
}

int
track_set_select(struct track *track, int mode, struct chan *sel, mux_select_func_t fsel)
{
	struct mux *mux = &track->mux[mode];
	struct chan *out = &track->out[mode];
	if (mux_init(mux, track->bay, sel, out, fsel) != 0) {
		err("mux_init failed");
		return -1;
	}

	return 0;
}


struct chan *
track_get_output(struct track *track, int mode)
{
	return &track->out[mode];
}

int
track_add_input(struct track *track, int mode, struct value key, struct chan *inp)
{
	struct mux *mux = &track->mux[mode];
	if (mux_add_input(mux, key, inp) != 0) {
		err("mux_add_input failed");
		return -1;
	}

	return 0;
}

static int
track_th_input_chan(struct track *track, struct chan *sel, struct chan *inp)
{
	/* Create all thread tracking modes */
	if (track_set_select(track, TRACK_TH_ANY, sel, thread_select_any) != 0) {
		err("track_select failed");
		return -1;
	}
	if (track_add_input(track, TRACK_TH_ANY, value_int64(0), inp) != 0) {
		err("track_add_input failed");
		return -1;
	}
	if (track_set_select(track, TRACK_TH_RUN, sel, thread_select_running) != 0) {
		err("track_select failed");
		return -1;
	}
	if (track_add_input(track, TRACK_TH_RUN, value_int64(0), inp) != 0) {
		err("track_add_input failed");
		return -1;
	}
	if (track_set_select(track, TRACK_TH_ACT, sel, thread_select_active) != 0) {
		err("track_select failed");
		return -1;
	}
	if (track_add_input(track, TRACK_TH_ACT, value_int64(0), inp) != 0) {
		err("track_add_input failed");
		return -1;
	}

	return 0;
}

int
track_connect_thread(struct track *tracks, struct chan *chans, const int *modes, struct chan *sel, int n)
{
	for (int i = 0; i < n; i++) {
		struct track *track = &tracks[i];

		if (track_th_input_chan(track, sel, &chans[i]) != 0) {
			err("track_th_input_chan failed");
			return -1;
		}

		/* Select the default output to PRV */
		track_set_default(track, modes[i]);
	}

	return 0;
}
