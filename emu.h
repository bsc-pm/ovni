#ifndef OVNI_EMU_H
#define OVNI_EMU_H

#include "ovni.h"
#include "uthash.h"
#include "parson.h"
#include <stdio.h>

/* Debug macros */

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
struct ovni_eproc;

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

	int index;
	int gindex;

	/* The process associated with this thread */
	struct ovni_eproc *proc;

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
	int pid;
	int index;
	int gindex;
	int appid;

	/* Path of the process tracedir */
	char dir[PATH_MAX];

	/* Threads */
	size_t nthreads;
	struct ovni_ethread thread[OVNI_MAX_THR];

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

struct ovni_cpu {
	/* Logical index: 0 to ncpus - 1 */
	int i;

	/* Physical id: as reported by lscpu(1) */
	int phyid;

	/* Global index for all CPUs */
	int gindex;

	enum ovni_cpu_state state;

	size_t last_nthreads;

	/* The threads the cpu is currently running */
	size_t nthreads;
	struct ovni_ethread *thread[OVNI_MAX_THR];
};

/* ----------------------- trace ------------------------ */

/* State of each loom on post-process */
struct ovni_loom {
	size_t nprocs;
	char hostname[OVNI_MAX_HOSTNAME];

	int max_ncpus;
	int max_phyid;
	int ncpus;
	struct ovni_cpu cpu[OVNI_MAX_CPU];

	int64_t clock_offset;

	/* Virtual CPU */
	struct ovni_cpu vcpu;

	struct ovni_eproc proc[OVNI_MAX_PROC];
};

struct ovni_trace {
	int nlooms;
	struct ovni_loom loom[OVNI_MAX_LOOM];
	int nstreams;
	struct ovni_stream *stream;
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
	int64_t clock_offset;
};

struct ovni_emu {
	struct ovni_trace trace;

	struct ovni_stream *cur_stream;
	struct ovni_ev *cur_ev;

	struct ovni_loom *cur_loom;
	struct ovni_eproc *cur_proc;
	struct ovni_ethread *cur_thread;

	struct nosv_task *cur_task;

	int64_t firstclock;
	int64_t lastclock;
	int64_t delta_time;

	FILE *prv_thread;
	FILE *prv_cpu;
	FILE *pcf_thread;
	FILE *pcf_cpu;

	char *clock_offset_file;
	char *tracedir;

	/* Total counters */
	int total_thread;
	int total_proc;
	int total_cpus;
};

/* Emulator function declaration */

void emu_emit(struct ovni_emu *emu);

void hook_pre_ovni(struct ovni_emu *emu);
void hook_emit_ovni(struct ovni_emu *emu);
void hook_post_ovni(struct ovni_emu *emu);

void hook_pre_nosv(struct ovni_emu *emu);
void hook_emit_nosv(struct ovni_emu *emu);
void hook_post_nosv(struct ovni_emu *emu);

struct ovni_cpu *emu_get_cpu(struct ovni_loom *loom, int cpuid);

struct ovni_ethread *emu_get_thread(struct ovni_eproc *proc, int tid);

void emu_emit_prv(struct ovni_emu *emu, int type, int val);

#endif /* OVNI_EMU_H */
