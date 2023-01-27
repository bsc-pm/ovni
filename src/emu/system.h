/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_SYSTEM_H
#define EMU_SYSTEM_H

#include "emu_args.h"
#include "emu_trace.h"
#include "emu_stream.h"
#include "loom.h"
#include "proc.h"
#include "thread.h"
#include "cpu.h"
#include "clkoff.h"
#include <stddef.h>

/* Map from stream to lpt */
struct lpt {
	struct emu_stream *stream; /* Redundancy */
	struct loom *loom;
	struct proc *proc;
	struct thread *thread;
};

struct system {
	/* Total counters */
	size_t nlooms;
	size_t nthreads;
	size_t nprocs;
	size_t ncpus; /* Physical */

	struct loom *looms;
	struct proc *procs;
	struct thread *threads;
	struct cpu *cpus;

	struct clkoff clkoff;
	struct emu_args *args;

	struct lpt *lpt;

	//struct model_ctx ctx;
};

int system_init(struct system *sys, struct emu_args *args, struct emu_trace *trace);
int system_connect(struct system *sys, struct bay *bay);
struct lpt *system_get_lpt(struct emu_stream *stream);
//struct emu_cpu *system_find_cpu(struct emu_loom *loom, int cpuid);
//int model_ctx_set(struct model_ctx *ctx, int model, void *data);
//int model_ctx_get(struct model_ctx *ctx, int model, void *data);

#endif /* EMU_SYSTEM_H */
