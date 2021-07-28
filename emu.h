#ifndef OVNI_EMU_H
#define OVNI_EMU_H

#include "ovni.h"
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

/* State of each emulated thread */
struct ovni_ethread {
	/* Emulated thread tid */
	pid_t tid;

	/* Stream file */
	FILE *f;

	enum ethread_state state;

	/* Thread stream */
	struct ovni_stream *stream;

	/* Current cpu */
	struct ovni_cpu *cpu;
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
	FILE *f;
	int tid;
	int thread;
	int proc;
	int loom;
	int loaded;
	int active;
	struct ovni_ev last;
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
};

/* Emulator function declaration */

void emu_emit(struct ovni_emu *emu);
void emu_process_ovni_ev(struct ovni_emu *emu);


int emu_cpu_find_thread(struct ovni_cpu *cpu, struct ovni_ethread *thread);

void emu_cpu_remove_thread(struct ovni_cpu *cpu, struct ovni_ethread *thread);

void emu_cpu_add_thread(struct ovni_cpu *cpu, struct ovni_ethread *thread);

struct ovni_cpu *emu_get_cpu(struct ovni_emu *emu, int cpuid);

struct ovni_ethread *emu_get_thread(struct ovni_emu *emu, int tid);

#endif /* OVNI_EMU_H */
