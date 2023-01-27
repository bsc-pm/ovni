/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MODEL_UST_H
#define MODEL_UST_H

/* The user-space thread "ust" execution model tracks the state of processes and
 * threads running in the CPUs by instrumenting the threads before and after
 * they are going to sleep. It just provides an approximate view of the real
 * execution by the kernel. */

#include "emu_model.h"

extern struct model_spec model_ust;

#include "chan.h"

enum ust_chan_type {
	UST_CHAN_FLUSH = 0,
	UST_CHAN_BURST,
	UST_CHAN_MAX,
};

struct ust_thread {
	struct chan chan[UST_CHAN_MAX];
};

#endif /* MODEL_UST_H */
