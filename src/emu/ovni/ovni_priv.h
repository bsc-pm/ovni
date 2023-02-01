/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef OVNI_PRIV_H
#define OVNI_PRIV_H

/* The user-space thread "ovni" execution model tracks the state of processes and
 * threads running in the CPUs by instrumenting the threads before and after
 * they are going to sleep. It jovni provides an approximate view of the real
 * execution by the kernel. */

#include "emu.h"
#include "chan.h"

enum ovni_chan_type {
	UST_CHAN_FLUSH = 0,
	UST_CHAN_BURST,
	UST_CHAN_MAX,
};

struct ovni_thread {
	struct chan chan[UST_CHAN_MAX];
};

int ovni_probe(struct emu *emu);
int ovni_event(struct emu *emu);

#endif /* OVNI_PRIV_H */
