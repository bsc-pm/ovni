/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ovni_model.h"

#include "emu.h"
#include "mux.h"

static int
cb_is_running(struct chan *in, void *ptr)
{
	struct chan *out = ptr;
	struct value value;

	if (chan_read(in, &value) != 0) {
		err("cb_is_running: chan_read %s failed\n", in->name);
		return -1;
	}

	if (value.type != VALUE_INT64)
		die("wrong value type\n");

	int st = value.i;
	if (st == TH_ST_RUNNING)
		value = value_int64(1);
	else
		value = value_int64(0);

	if (chan_set(out, value) != 0) {
		err("cb_is_running: chan_set %s failed\n", out->name);
		return -1;
	}
	
	return 0;
}

static int
cb_is_active(struct chan *in, void *ptr)
{
	struct chan *out = ptr;
	struct value value;

	if (chan_read(in, &value) != 0) {
		err("cb_is_running: chan_read %s failed\n", in->name);
		return -1;
	}

	if (value.type != VALUE_INT64)
		die("wrong value type\n");

	int st = value.i;
	if (st == TH_ST_RUNNING || st == TH_ST_COOLING || st == TH_ST_WARMING)
		value = value_int64(1);
	else
		value = value_int64(0);

	if (chan_set(out, value) != 0) {
		err("cb_is_running: chan_set %s failed\n", out->name);
		return -1;
	}
	
	return 0;
}

static struct chan *
find_thread_chan(struct bay *bay, long th_gindex, char *name)
{
	char fullname[MAX_CHAN_NAME];
	sprintf(fullname, "ovni.thread%ld.%s", th_gindex, name);
	return bay_find(bay, fullname);
}

static void
track_thread_state(struct bay *bay, long th_gindex)
{
	struct chan *state = find_thread_chan(bay, th_gindex, "state");
	struct chan *is_running = find_thread_chan(bay, th_gindex, "is_running");
	struct chan *is_active = find_thread_chan(bay, th_gindex, "is_active");

	if (bay_add_cb(bay, BAY_CB_DIRTY, state, cb_is_running, is_running) != 0)
		die("bay_add_cb failed\n");
	if (bay_add_cb(bay, BAY_CB_DIRTY, state, cb_is_active, is_active) != 0)
		die("bay_add_cb failed\n");
}

int
ovni_model_connect(void *ptr)
{
	struct emu *emu = emu_get(ptr);

	for (size_t i = 0; i < emu->system.nthreads; i++)
		track_thread_state(&emu->bay, i);

	return 0;
}
