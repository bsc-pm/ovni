/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_SYSTEM_H
#define EMU_SYSTEM_H

#include "emu_args.h"
#include "trace.h"
#include "stream.h"
#include "loom.h"
#include "proc.h"
#include "thread.h"
#include "cpu.h"
#include "clkoff.h"
#include "recorder.h"
#include <stddef.h>

/* Map from stream to lpt */
struct lpt {
	struct stream *stream; /* Redundancy */
	struct loom *loom;
	struct proc *proc;
	struct thread *thread;
};

struct system {
	/* Total counters */
	size_t nlooms;
	size_t nthreads;
	size_t nprocs;
	size_t ncpus; /* Including virtual cpus */

	struct loom *looms;
	struct proc *procs;
	struct thread *threads;
	struct cpu *cpus;

	struct clkoff clkoff;
	struct emu_args *args;

	struct lpt *lpt;
};

int system_init(struct system *sys, struct emu_args *args, struct trace *trace);
int system_connect(struct system *sys, struct bay *bay, struct recorder *rec);
struct lpt *system_get_lpt(struct stream *stream);

#endif /* EMU_SYSTEM_H */
