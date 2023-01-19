/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_SYSTEM_H
#define EMU_SYSTEM_H

#include "emu_trace.h"
#include "parson.h"
#include "ovni.h"
#include <stddef.h>

#define MAX_CPU_NAME 32

struct emu_cpu;
struct emu_thread;
struct emu_proc;
struct emu_loom;
struct emu_system;

enum emu_cpu_state {
	CPU_ST_UNKNOWN,
	CPU_ST_READY,
};

#define MAX_MODELS 256

struct model_ctx {
	void *data[MAX_MODELS];
};

struct emu_cpu {
	size_t gindex; /* In the system */
	char name[PATH_MAX];

	/* Logical index: 0 to ncpus - 1 */
	int i;

	/* Physical id: as reported by lscpu(1) */
	int phyid;

	enum emu_cpu_state state;

	/* The loom of the CPU */
	struct emu_loom *loom;

	size_t nthreads;
	struct emu_thread *thread; /* List of threads assigned to this CPU */

	size_t nrunning_threads;
	struct emu_thread *th_running; /* One */

	size_t nactive_threads;
	struct emu_thread *th_active;

	int is_virtual;

	/* Global list */
	struct emu_cpu *next;
	struct emu_cpu *prev;

	struct model_ctx ctx;
};

/* Emulated thread runtime status */
enum emu_thread_state {
	TH_ST_UNKNOWN,
	TH_ST_RUNNING,
	TH_ST_PAUSED,
	TH_ST_DEAD,
	TH_ST_COOLING,
	TH_ST_WARMING,
};

struct emu_thread {
	size_t gindex; /* In the system */

	char name[PATH_MAX];
	char path[PATH_MAX];
	char relpath[PATH_MAX];

	int tid;
	int index; /* In loom */

	/* The process associated with this thread */
	struct emu_proc *proc;

	enum emu_thread_state state;
	int is_running;
	int is_active;

	/* Stream linked to this thread */
	struct emu_stream *stream;

	/* Current cpu, NULL if not unique affinity */
	struct emu_cpu *cpu;

	/* Linked list of threads in each CPU */
	struct emu_thread *cpu_prev;
	struct emu_thread *cpu_next;

	/* Local list */
	struct emu_thread *lprev;
	struct emu_thread *lnext;

	/* Global list */
	struct emu_thread *gnext;
	struct emu_thread *gprev;

	struct model_ctx ctx;
};

/* State of each emulated process */
struct emu_proc {
	size_t gindex;

	char name[PATH_MAX]; /* Proc directory name */
	char path[PATH_MAX];
	char relpath[PATH_MAX];

	int pid;
	int index;
	int appid;
	int rank;

	struct emu_loom *loom;

	JSON_Value *meta;

	int nthreads;
	struct emu_thread *threads;

	/* Local list */
	struct emu_proc *lnext;
	struct emu_proc *lprev;

	/* Global list */
	struct emu_proc *gnext;
	struct emu_proc *gprev;

	struct model_ctx ctx;
};

struct emu_loom {
	size_t gindex;

	char name[PATH_MAX]; /* Loom directory name */
	char path[PATH_MAX];
	char relpath[PATH_MAX];  /* Relative to tracedir */

	char hostname[OVNI_MAX_HOSTNAME];

	size_t max_ncpus;
	size_t max_phyid;
	size_t ncpus;
	size_t offset_ncpus;
	struct emu_cpu *cpu;
	int rank_enabled;

	int64_t clock_offset;

	/* Virtual CPU */
	struct emu_cpu vcpu;

	/* Local list */
	size_t nprocs;
	struct emu_proc *procs;

	/* Global list */
	struct emu_loom *next;
	struct emu_loom *prev;

	struct model_ctx ctx;
};

struct emu_system {
	/* Total counters */
	size_t nlooms;
	size_t nthreads;
	size_t nprocs;
	size_t ncpus; /* Physical */

	struct emu_loom *looms;
	struct emu_proc *procs;
	struct emu_thread *threads;
	struct emu_cpu *cpus;

	/* From current event */
	struct emu_loom *cur_loom;
	struct emu_proc *cur_proc;
	struct emu_thread *cur_thread;

	struct model_ctx ctx;
};

int emu_system_load(struct emu_system *system, struct emu_trace *trace);

int model_ctx_set(struct model_ctx *ctx, int model, void *data);
int model_ctx_get(struct model_ctx *ctx, int model, void *data);

#endif /* EMU_SYSTEM_H */
