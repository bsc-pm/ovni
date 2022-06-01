/*
 * Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef OVNI_EMU_H
#define OVNI_EMU_H

#include <stdio.h>

#include "common.h"
#include "ovni.h"
#include "uthash.h"
#include "parson.h"
#include "heap.h"
#include "pcf.h"

/* Emulated thread runtime status */
enum ethread_state {
	TH_ST_UNKNOWN,
	TH_ST_RUNNING,
	TH_ST_PAUSED,
	TH_ST_DEAD,
	TH_ST_COOLING,
	TH_ST_WARMING,
};

enum nosv_task_state {
	TASK_ST_CREATED,
	TASK_ST_RUNNING,
	TASK_ST_PAUSED,
	TASK_ST_DEAD,
};

enum ovni_state {
	ST_OVNI_FLUSHING = 1,
};

enum error_values {
	ST_BAD = 666,
	ST_TOO_MANY_TH = 777,
};

enum nosv_ss_values {
	ST_NULL = 0,
	ST_NOSV_SCHED_HUNGRY = 6,
	ST_NOSV_SCHED_SERVING,
	ST_NOSV_SCHED_SUBMITTING,
	ST_NOSV_MEM_ALLOCATING,
	ST_NOSV_MEM_FREEING,
	ST_NOSV_TASK_RUNNING,
	ST_NOSV_API_SUBMIT,
	ST_NOSV_API_PAUSE,
	ST_NOSV_API_YIELD,
	ST_NOSV_API_WAITFOR,
	ST_NOSV_API_SCHEDPOINT,
	ST_NOSV_ATTACH,
	ST_NOSV_WORKER,
	ST_NOSV_DELEGATE,

	EV_NOSV_SCHED_RECV,
	EV_NOSV_SCHED_SEND,
	EV_NOSV_SCHED_SELF,
};

enum tampi_state {
	ST_TAMPI_SEND = 1,
	ST_TAMPI_RECV = 2,
	ST_TAMPI_ISEND = 3,
	ST_TAMPI_IRECV = 4,
	ST_TAMPI_WAIT = 5,
	ST_TAMPI_WAITALL = 6,
};

enum openmp_state {
	ST_OPENMP_TASK = 1,
	ST_OPENMP_PARALLEL = 2,
};

enum nodes_state {
	ST_NODES_REGISTER = 1,
	ST_NODES_UNREGISTER = 2,
	ST_NODES_IF0_WAIT = 3,
	ST_NODES_IF0_INLINE = 4,
	ST_NODES_TASKWAIT = 5,
	ST_NODES_CREATE = 6,
	ST_NODES_SUBMIT = 7,
	ST_NODES_SPAWN = 8,
};

enum kernel_cs_state {
	ST_KERNEL_CSOUT = 3,
};

struct ovni_ethread;
struct ovni_eproc;

struct nosv_task {
	int id;
	int type_id;
	struct ovni_ethread *thread;
	enum nosv_task_state state;
	UT_hash_handle hh;

	/* List handle for nested task support */
	struct nosv_task *next;
	struct nosv_task *prev;
};

struct nosv_task_type {
	int gid; /* Global identifier */
	int id; /* Per-process identifier */
	char label[MAX_PCF_LABEL];
	UT_hash_handle hh;
};

#define MAX_CHAN_STACK 128

enum chan_track {
	/* The channel is manually controlled. */
	CHAN_TRACK_NONE = 0,

	/* Enables the channel when the thread is running only. */
	CHAN_TRACK_TH_RUNNING,

	/* The thread active tracking mode a enables the channel when
	 * the thread is running, cooling or warming. Otherwise the
	 * channel is disabled. */
	CHAN_TRACK_TH_ACTIVE,
};

enum chan {
	CHAN_OVNI_PID,
	CHAN_OVNI_TID,
	CHAN_OVNI_NRTHREADS,
	CHAN_OVNI_STATE,
	CHAN_OVNI_APPID,
	CHAN_OVNI_CPU,
	CHAN_OVNI_FLUSH,

	CHAN_NOSV_TASKID,
	CHAN_NOSV_TYPEID,
	CHAN_NOSV_APPID,
	CHAN_NOSV_SUBSYSTEM,
	CHAN_NOSV_RANK,

	CHAN_TAMPI_MODE,
	CHAN_OPENMP_MODE,
	CHAN_NODES_SUBSYSTEM,

	CHAN_KERNEL_CS,

	CHAN_MAX
};

enum chan_type {
	CHAN_TH = 0,
	CHAN_CPU = 1,
	CHAN_MAXTYPE = 2,
};

enum chan_dirty {
	CHAN_CLEAN = 0,

	/* The channel is dirty because it has been enabled or disabled */
	CHAN_DIRTY_ACTIVE = 1,

	/* The channel is dirty because it changed the state */
	CHAN_DIRTY_VALUE = 2,
};

static const int chan_to_prvtype[CHAN_MAX][CHAN_MAXTYPE] = {
	/* FIXME: Use odd/even identifiers for thread and cpu */
	/* Channel                 TH   CPU */
	[CHAN_OVNI_PID]        = { 10,  60  },
	[CHAN_OVNI_TID]        = { 11,  61  },
	[CHAN_OVNI_NRTHREADS]  = { -1,  62  },
	[CHAN_OVNI_STATE]      = { 13,  -1  },
	[CHAN_OVNI_APPID]      = { 14,  64  }, /* Not used */
	[CHAN_OVNI_CPU]        = { 15,  -1  },
	[CHAN_OVNI_FLUSH]      = { 16,  66  },
	[CHAN_NOSV_TASKID]     = { 20,  70  },
	[CHAN_NOSV_TYPEID]     = { 21,  71  },
	[CHAN_NOSV_APPID]      = { 22,  72  },
	[CHAN_NOSV_SUBSYSTEM]  = { 23,  73  },
	[CHAN_NOSV_RANK]       = { 24,  74  },
	[CHAN_TAMPI_MODE]      = { 30,  80  },
	[CHAN_OPENMP_MODE]     = { 40,  90  },
	[CHAN_NODES_SUBSYSTEM] = { 50, 100  },
	[CHAN_KERNEL_CS]       = { 55, 105  },
};

#define CHAN_PRV_TH(id) chan_to_prvtype[id][CHAN_TH]
#define CHAN_PRV_CPU(id) chan_to_prvtype[id][CHAN_CPU]

struct ovni_chan {
	/* Channel id */
	enum chan id;

	/* Number of states in the stack */
	int n;

	/* Stack of states */
	int stack[MAX_CHAN_STACK];

	/* 1 if enabled, 0 if not. */
	int enabled;

	/* What state should be shown in errors */
	int badst;

	/* Last state emitted (-1 otherwise) */
	int lastst;

	/* Punctual event: -1 if not used */
	int ev;

	/* Emit events of this type */
	int type;

	/* A pointer to a clock to sample the time */
	int64_t *clock;

	/* The time of the last state or event */
	int64_t t;

	/* Paraver row */
	int row;

	/* Type of dirty */
	enum chan_dirty dirty;

	/* Where should the events be written to? */
	FILE *prv;

	/* What should cause the channel to become disabled? */
	enum chan_track track;

	/* The thread associated with the channel if any */
	struct ovni_ethread *thread;

	/* The CPU associated with the channel if any */
	struct ovni_cpu *cpu;

	struct ovni_chan **update_list;

	/* Used when the channel is a list */
	struct ovni_chan *prev;
	struct ovni_chan *next;
};

#define MAX_BURSTS 100

/* State of each emulated thread */
struct ovni_ethread {
	/* Emulated thread tid */
	pid_t tid;

	int index;
	int gindex;

	/* The process associated with this thread */
	struct ovni_eproc *proc;

	enum ethread_state state;
	int is_running;
	int is_active;

	/* Thread stream */
	struct ovni_stream *stream;

	/* Current cpu */
	struct ovni_cpu *cpu;

	/* FIXME: Use a table with registrable pointers to custom data
	 * structures */

	/* nOS-V stack of tasks: points to the runnable task. */
	struct nosv_task *task_stack;

	/* Channels are used to output the emulator state in PRV */
	struct ovni_chan chan[CHAN_MAX];

	/* Burst times */
	int nbursts;
	int64_t burst_time[MAX_BURSTS];

	/* These pointers keep a linked list of threads in each CPU */
	struct ovni_ethread *prev;
	struct ovni_ethread *next;

	/* Trace file path */
	char tracefile[PATH_MAX];
};

/* State of each emulated process */
struct ovni_eproc {
	int pid;
	int index;
	int gindex;
	int appid;
	int rank;

	/* The loom of the current process */
	struct ovni_loom *loom;

	/* Path of the process tracedir */
	char dir[PATH_MAX];

	/* Threads */
	size_t nthreads;
	struct ovni_ethread *thread;

	JSON_Value *meta;

	/* ------ Subsystem specific data --------*/
	/* TODO: Use dynamic allocation */

	struct nosv_task_type *types;
	struct nosv_task *tasks;
};


/* ------------------ emulation ---------------- */

enum ovni_cpu_type {
	CPU_REAL,
	CPU_VIRTUAL,
};

enum ovni_cpu_state {
	CPU_ST_UNKNOWN,
	CPU_ST_READY,
};

#define MAX_CPU_NAME 32

struct ovni_cpu {
	/* Logical index: 0 to ncpus - 1 */
	int i;

	/* Physical id: as reported by lscpu(1) */
	int phyid;

	/* Global index for all CPUs */
	int gindex;

	enum ovni_cpu_state state;

	/* The loom of the CPU */
	struct ovni_loom *loom;

	/* CPU channels */
	struct ovni_chan chan[CHAN_MAX];

	/* The threads assigned to this CPU */
	size_t nthreads;
	struct ovni_ethread *thread;

	/* Running threads */
	size_t nrunning_threads;
	struct ovni_ethread *th_running;

	/* Active threads (not paused or dead) */
	size_t nactive_threads;
	struct ovni_ethread *th_active;

	/* Cpu name as shown in paraver row */
	char name[MAX_CPU_NAME];

	/* Is this a virtual CPU? */
	int virtual;
};

/* ----------------------- trace ------------------------ */

/* State of each loom on post-process */
struct ovni_loom {
	size_t nprocs;
	char hostname[OVNI_MAX_HOSTNAME];

	size_t max_ncpus;
	size_t max_phyid;
	size_t ncpus;
	size_t offset_ncpus;
	struct ovni_cpu *cpu;
	int rank_enabled;

	int64_t clock_offset;

	/* Virtual CPU */
	struct ovni_cpu vcpu;

	struct ovni_eproc *proc;
};

#define MAX_VIRTUAL_EVENTS 16

struct ovni_trace {
	size_t nlooms;
	struct ovni_loom *loom;

	size_t nstreams;
	struct ovni_stream *stream;
};

struct ovni_stream {
	uint8_t *buf;
	size_t size;
	size_t offset;

	int tid;
	struct ovni_loom *loom;
	struct ovni_eproc *proc;
	struct ovni_ethread *thread;
	int loaded;
	int active;

	double progress;

	struct ovni_ev *cur_ev;
	int64_t lastclock;
	int64_t clock_offset;

	heap_node_t hh;
};

struct ovni_emu {
	struct ovni_trace trace;

	struct ovni_stream *cur_stream;
	struct ovni_ev *cur_ev;

	struct ovni_loom *cur_loom;
	struct ovni_eproc *cur_proc;
	struct ovni_ethread *cur_thread;

	/* Indexed by gindex */
	struct ovni_ethread **global_thread;
	struct ovni_cpu **global_cpu;

	/* Global processed size and offset of all streams */
	size_t global_size;
	size_t global_offset;
	double start_emulation_time;

	int64_t firstclock;
	int64_t lastclock;
	int64_t delta_time;

	/* Counters for statistics */
	int64_t nev_processed;

	/* Be strict */
	int enable_linter;

	FILE *prv_thread;
	FILE *prv_cpu;

	struct pcf_file pcf[CHAN_MAXTYPE];

	char *clock_offset_file;
	char *tracedir;

	/* Total counters */
	size_t total_nthreads;
	size_t total_nprocs;
	size_t total_ncpus;

	uint32_t nosv_type_counter;

	/* Keep a list of dirty channels for the CPUs and threads */
	struct ovni_chan *cpu_chan;
	struct ovni_chan *th_chan;

	heap_head_t sorted_stream;
};

/* Emulator function declaration */

void hook_init_ovni(struct ovni_emu *emu);
void hook_pre_ovni(struct ovni_emu *emu);

void hook_init_nosv(struct ovni_emu *emu);
void hook_pre_nosv(struct ovni_emu *emu);
void hook_end_nosv(struct ovni_emu *emu);

void hook_init_tampi(struct ovni_emu *emu);
void hook_pre_tampi(struct ovni_emu *emu);

void hook_init_openmp(struct ovni_emu *emu);
void hook_pre_openmp(struct ovni_emu *emu);

void hook_init_nodes(struct ovni_emu *emu);
void hook_pre_nodes(struct ovni_emu *emu);

void hook_init_kernel(struct ovni_emu *emu);
void hook_pre_kernel(struct ovni_emu *emu);

struct ovni_cpu *emu_get_cpu(struct ovni_loom *loom, int cpuid);

struct ovni_ethread *emu_get_thread(struct ovni_eproc *proc, int tid);

void emu_cpu_update_chan(struct ovni_cpu *cpu, struct ovni_chan *cpu_chan);

#endif /* OVNI_EMU_H */
