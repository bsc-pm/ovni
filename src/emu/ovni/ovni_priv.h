/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef OVNI_PRIV_H
#define OVNI_PRIV_H

/* The user-space thread "ovni" execution model tracks the state of processes and
 * threads running in the CPUs by instrumenting the threads before and after
 * they are going to sleep. It just provides an approximate view of the real
 * execution by the kernel. */

#include "emu.h"
#include "mark.h"
#include "model_cpu.h"
#include "model_thread.h"
#include <stdint.h>

enum ovni_chan_type {
	CH_FLUSH = 0,
	CH_MAX,
};

enum ovni_flusing_st {
	ST_FLUSHING = 1,
};

#define MAX_BURSTS 100

struct ovni_thread {
	struct model_thread m;

	/* Burst times */
	int nbursts;
	int64_t burst_time[MAX_BURSTS];

	int64_t flush_start;

	struct ovni_mark_thread mark;
};

struct ovni_cpu {
	struct model_cpu m;
	struct ovni_mark_cpu mark;
};

struct ovni_emu {
	struct ovni_mark_emu mark;
};

int model_ovni_probe(struct emu *emu);
int model_ovni_create(struct emu *emu);
int model_ovni_connect(struct emu *emu);
int model_ovni_event(struct emu *emu);
int model_ovni_finish(struct emu *emu);

#endif /* OVNI_PRIV_H */
