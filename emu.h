#ifndef OVNI_EMU_H
#define OVNI_EMU_H

#include "ovni.h"
#include "uthash.h"
#include <stdio.h>

/* Debug macros */

#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
# define dbg(...) fprintf(stderr, __VA_ARGS__);
#else
# define dbg(...)
#endif

#define err(...) fprintf(stderr, __VA_ARGS__);

/* Emulated thread runtime status */
enum ethread_state {
	TH_ST_UNKNOWN,
	TH_ST_RUNNING,
	TH_ST_PAUSED,
	TH_ST_DEAD,
};

enum nosv_task_state {
	TASK_ST_CREATED,
	TASK_ST_RUNNING,
	TASK_ST_PAUSED,
	TASK_ST_DEAD,
};

struct ovni_ethread;

struct nosv_task {
	int id;
	int type_id;
	struct ovni_ethread *thread;
	enum nosv_task_state state;
	UT_hash_handle hh;
};

struct nosv_task_type {
	int id;
	const char *label;
	UT_hash_handle hh;
};

/* State of each emulated thread */
struct ovni_ethread {
	/* Emulated thread tid */
	pid_t tid;

	/* Stream fd */
	int stream_fd;

	enum ethread_state state;

	/* Thread stream */
	struct ovni_stream *stream;

	/* Current cpu */
	struct ovni_cpu *cpu;

	/* FIXME: Use a table with registrable pointers to custom data
	 * structures */

	/* nosv task */
	struct nosv_task *task;
};

/* State of each emulated process */
struct ovni_eproc {
	/* Monotonic counter for process index */
	/* TODO: Use pid? */
	int proc;

	/* Path of the process tracedir */
	char dir[PATH_MAX];

	/* Threads */
	size_t nthreads;
	struct ovni_ethread thread[OVNI_MAX_THR];
};

/* ----------------------- trace ------------------------ */

/* State of each loom on post-process */
struct ovni_loom {
	size_t nprocs;
	struct ovni_eproc proc[OVNI_MAX_PROC];
};

struct ovni_stream {
	uint8_t *buf;
	size_t size;
	size_t offset;

	int tid;
	int thread;
	int proc;
	int loom;
	int loaded;
	int active;
	struct ovni_ev *cur_ev;
	uint64_t lastclock;
};

struct ovni_trace {
	int nlooms;
	struct ovni_loom loom[OVNI_MAX_LOOM];
	int nstreams;
	struct ovni_stream *stream;
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

struct ovni_cpu {
	/* Physical id */
	int cpu_id;

	/* Position in emu->cpu */
	int index;

	enum ovni_cpu_state state;
	enum ovni_cpu_type type;

	size_t last_nthreads;

	/* The threads the cpu is currently running */
	size_t nthreads;
	struct ovni_ethread *thread[OVNI_MAX_THR];
};

struct ovni_emu {
	struct ovni_trace trace;

	/* Physical CPUs */
	int max_ncpus;
	int ncpus;
	struct ovni_cpu cpu[OVNI_MAX_CPU];
	int cpuind[OVNI_MAX_CPU];

	/* Virtual CPU */
	struct ovni_cpu vcpu;

	struct ovni_stream *cur_stream;
	struct ovni_ev *cur_ev;

	struct ovni_loom *cur_loom;
	struct ovni_eproc *cur_proc;
	struct ovni_ethread *cur_thread;

	uint64_t lastclock;
	int64_t delta_time;

	struct nosv_task *cur_task;
};

/* Emulator function declaration */

void emu_emit(struct ovni_emu *emu);

void hook_pre_ovni(struct ovni_emu *emu);
void hook_view_ovni(struct ovni_emu *emu);
void hook_post_ovni(struct ovni_emu *emu);

void hook_pre_nosv(struct ovni_emu *emu);
void hook_view_nosv(struct ovni_emu *emu);
void hook_post_nosv(struct ovni_emu *emu);

struct ovni_cpu *emu_get_cpu(struct ovni_emu *emu, int cpuid);

struct ovni_ethread *emu_get_thread(struct ovni_emu *emu, int tid);

#endif /* OVNI_EMU_H */
