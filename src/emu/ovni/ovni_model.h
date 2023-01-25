/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef OVNI_MODEL_H
#define OVNI_MODEL_H

/* The emulator ovni module provides the execution model by
 * tracking the thread state and which threads run in each
 * CPU */

/* Cannot depend on emu.h */
#include "emu_model.h"
#include "chan.h"
#include <stdint.h>

enum ovni_flushing_state {
	ST_OVNI_FLUSHING = 1,
};

enum ovni_chan {
	CHAN_OVNI_PID = 0,
	CHAN_OVNI_TID,
	CHAN_OVNI_NRTHREADS,
	CHAN_OVNI_STATE,
	CHAN_OVNI_APPID,
	CHAN_OVNI_CPU,
	CHAN_OVNI_FLUSH,
	CHAN_OVNI_MAX,
};

#define MAX_BURSTS 100

struct ovni_cpu {
	/* CPU channels */
	struct chan chan[CHAN_OVNI_MAX];
	struct chan pid_running;
	struct chan tid_running;
	struct chan nthreads_running;
};

struct ovni_thread {
	struct chan chan[CHAN_OVNI_MAX];

	struct chan cpu;
	struct chan flush;

	/* Burst times */
	int nbursts;
	int64_t burst_time[MAX_BURSTS];
};

extern struct model_spec ovni_model_spec;

int ovni_model_probe(void *ptr);
int ovni_model_create(void *ptr);
int ovni_model_connect(void *ptr);
int ovni_model_event(void *ptr);

#endif /* OVNI_MODEL_H */
