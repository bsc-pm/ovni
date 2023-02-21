/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef CPU_H
#define CPU_H

struct cpu; /* Needed for thread */

#include "thread.h"
#include "chan.h"
#include "track.h"
#include "bay.h"
#include "uthash.h"
#include "recorder.h"
#include "extend.h"
#include <linux/limits.h>

enum cpu_chan {
	CPU_CHAN_NRUN = 0,
	CPU_CHAN_PID,
	CPU_CHAN_TID,
	CPU_CHAN_APPID,
	CPU_CHAN_THRUN, /* gindex */
	CPU_CHAN_THACT, /* gindex */
	CPU_CHAN_MAX,
};

struct cpu {
	int64_t gindex; /* In the system */
	char name[PATH_MAX];
	int is_init;

	/* Logical index: 0 to ncpus - 1 */
	//int index;

	/* Physical id: as reported by lscpu(1) */
	int phyid;

	/* Required to find threads that can run in this CPU */
	struct loom *loom;

	size_t nthreads;
	size_t nth_running;
	size_t nth_active;
	struct thread *threads; /* List of threads assigned to this CPU */
	struct thread *th_running; /* Unique thread or NULL */
	struct thread *th_active; /* Unique thread or NULL */

	int is_virtual;

	/* Loom list sorted by phyid */
	struct cpu *lnext;
	struct cpu *lprev;

	/* Global list */
	struct cpu *next;
	struct cpu *prev;

	/* Channels */
	struct chan chan[CPU_CHAN_MAX];

	struct extend ext;

	UT_hash_handle hh; /* CPUs in the loom */
};

        void cpu_init_begin(struct cpu *cpu, int phyid, int is_virtual);
USE_RET int cpu_get_phyid(struct cpu *cpu);
        void cpu_set_gindex(struct cpu *cpu, int64_t gindex);
        void cpu_set_loom(struct cpu *cpu, struct loom *loom);
        void cpu_set_name(struct cpu *cpu, const char *name);
USE_RET int cpu_init_end(struct cpu *cpu);
USE_RET int cpu_connect(struct cpu *cpu, struct bay *bay, struct recorder *rec);

USE_RET int cpu_update(struct cpu *cpu);
USE_RET int cpu_add_thread(struct cpu *cpu, struct thread *thread);
USE_RET int cpu_remove_thread(struct cpu *cpu, struct thread *thread);
USE_RET int cpu_migrate_thread(struct cpu *cpu, struct thread *thread, struct cpu *newcpu);

struct chan *cpu_get_th_chan(struct cpu *cpu, enum track_th mode);
struct pcf_value *cpu_add_to_pcf_type(struct cpu *cpu, struct pcf_type *type);

#endif /* CPU_H */
