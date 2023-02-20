/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef OVNI_PRIV_H
#define OVNI_PRIV_H

/* The user-space thread "ovni" execution model tracks the state of processes and
 * threads running in the CPUs by instrumenting the threads before and after
 * they are going to sleep. It just provides an approximate view of the real
 * execution by the kernel. */

#include "emu.h"
#include "chan.h"
#include <stdint.h>

enum ovni_chan_type {
	CH_FLUSH = 0,
	CH_MAX,
};

#define MAX_BURSTS 100

struct ovni_thread {
	struct chan ch[CH_MAX];
	struct chan ch_run[CH_MAX];
	struct chan mux_run[CH_MAX];

	/* Burst times */
	int nbursts;
	int64_t burst_time[MAX_BURSTS];
};

struct ovni_cpu {
	struct chan ch[CH_MAX];
	struct mux mux[CH_MAX];
};

int ovni_probe(struct emu *emu);
int ovni_create(struct emu *emu);
int ovni_event(struct emu *emu);
int ovni_finish(struct emu *emu);

#endif /* OVNI_PRIV_H */
