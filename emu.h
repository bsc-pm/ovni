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
	TH_ST_COOLING,
	TH_ST_WARMING,
};

enum nosv_task_state {
	TASK_ST_CREATED,
	TASK_ST_RUNNING,
	TASK_ST_PAUSED,
	TASK_ST_DEAD,
};

enum nosv_ss_values {
	ST_NULL = 0,
	ST_NOSV_SCHED_HUNGRY = 6,
	ST_NOSV_SCHED_SERVING = 7,
	ST_NOSV_SCHED_SUBMITTING = 8,
	ST_NOSV_MEM_ALLOCATING = 9,
	ST_NOSV_TASK_RUNNING = 10,
	ST_NOSV_CODE = 11,
	ST_NOSV_API_SUBMIT = 12,
	ST_NOSV_API_PAUSE = 13,
	ST_NOSV_API_YIELD = 14,
	ST_NOSV_API_WAITFOR = 15,
	ST_NOSV_API_SCHEDPOINT = 16,

	EV_NOSV_SCHED_RECV = 50,
	EV_NOSV_SCHED_SEND = 51,
	EV_NOSV_SCHED_SELF = 52,

	ST_BAD = 666,
};

enum nosv_tampi_state {
	ST_TAMPI_SEND = 1,
	ST_TAMPI_RECV = 2,
	ST_TAMPI_ISEND = 3,
	ST_TAMPI_IRECV = 4,
	ST_TAMPI_WAIT = 5,
	ST_TAMPI_WAITALL = 6,
};

enum nosv_openmp_state {
	ST_OPENMP_TASK = 1,
	ST_OPENMP_PARALLEL = 2,
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

#define MAX_CHAN_STACK 128

enum chan_track {
	CHAN_TRACK_NONE = 0,
	CHAN_TRACK_TH_RUNNING,
	CHAN_TRACK_TH_UNPAUSED,
};

struct ovni_chan {
	/* Number of states in the stack */
	int n;

	/* Stack of states */
	int stack[MAX_CHAN_STACK];

	/* 1 if enabled, 0 if not. */
	int enabled;

	/* What state should be shown in errors */
	int badst;

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

	/* 1 if channel needs flush to PRV */
	int dirty;

	/* Where should the events be written to? */
	FILE *prv;

	/* What should cause the channel to become disabled? */
	enum chan_track track;
};

enum chan {
	CHAN_OVNI_PID,
	CHAN_OVNI_TID,
	CHAN_OVNI_NTHREADS,
	CHAN_OVNI_STATE,
	CHAN_OVNI_APPID,
	CHAN_OVNI_CPU,

	CHAN_NOSV_TASKID,
	CHAN_NOSV_TYPEID,
	CHAN_NOSV_APPID,
	CHAN_NOSV_SUBSYSTEM,

	CHAN_TAMPI_MODE,
	CHAN_OPENMP_MODE,

	CHAN_MAX
};

/* Same order as `enum chan` */
const static int chan_to_prvtype[CHAN_MAX][3] = {
	/* Channel		TH  CPU */
	{ CHAN_OVNI_PID,	10, 60 },
	{ CHAN_OVNI_TID,	11, 61 },
	{ CHAN_OVNI_NTHREADS,	-1, 62 },
	{ CHAN_OVNI_STATE,	13, 63 },
	{ CHAN_OVNI_APPID,	14, 64 },
	{ CHAN_OVNI_CPU,	15, -1 },

	{ CHAN_NOSV_TASKID,	20, 70 },
	{ CHAN_NOSV_TYPEID,	21, 71 },
	{ CHAN_NOSV_APPID,	22, 72 },
	{ CHAN_NOSV_SUBSYSTEM,	23, 73 },

	{ CHAN_TAMPI_MODE,	30, 80 },

	{ CHAN_OPENMP_MODE,	40, 90 },
};

///* All PRV event types */
//enum prv_type {
//	/* Rows are CPUs */
//	PTC_PROC_PID      = 10,
//	PTC_THREAD_TID    = 11,
//	PTC_NTHREADS      = 12,
//	PTC_TASK_ID       = 20,
//	PTC_TASK_TYPE_ID  = 21,
//	PTC_APP_ID        = 30,
//	PTC_SUBSYSTEM     = 31,
//
//	/* Rows are threads */
//	PTT_THREAD_STATE  = 60,
//	PTT_THREAD_TID    = 61,
//	PTT_SUBSYSTEM     = 62,
//
//	PTT_TASK_ID       = 63,
//	PTT_TASK_TYPE_ID  = 64,
//	PTT_TASK_APP_ID   = 65,
//};

#define MAX_BURSTS 100

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

	/* Channels are used to output the emulator state in PRV */
	struct ovni_chan chan[CHAN_MAX];

	/* Burst times */
	int nbursts;
	int64_t burst_time[MAX_BURSTS];
};

/* State of each emulated process */
struct ovni_eproc {
	int pid;
	int index;
	int gindex;
	int appid;

	/* The loom of the current process */
	struct ovni_loom *loom;

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

#define MAX_CPU_NAME 32

struct ovni_cpu {
	/* Logical index: 0 to ncpus - 1 */
	int i;

	/* Physical id: as reported by lscpu(1) */
	int phyid;

	/* Global index for all CPUs */
	int gindex;

	enum ovni_cpu_state state;

	/* CPU channels */
	struct ovni_chan chan[CHAN_MAX];

	size_t last_nthreads;

	/* 1 if the cpu has updated is threads, 0 if not */
	int updated;

	/* The threads the cpu is currently running */
	size_t nthreads;
	struct ovni_ethread *thread[OVNI_MAX_THR];

	/* Cpu name as shown in paraver row */
	char name[MAX_CPU_NAME];
};

/* ----------------------- trace ------------------------ */

/* State of each loom on post-process */
struct ovni_loom {
	size_t nprocs;
	char hostname[OVNI_MAX_HOSTNAME];

	int max_ncpus;
	int max_phyid;
	int ncpus;
	int offset_ncpus;
	struct ovni_cpu cpu[OVNI_MAX_CPU];

	int64_t clock_offset;

	/* Virtual CPU */
	struct ovni_cpu vcpu;

	struct ovni_eproc proc[OVNI_MAX_PROC];

	/* Keep a list of updated cpus */
	int nupdated_cpus;
	struct ovni_cpu *updated_cpu[OVNI_MAX_CPU];
};

#define MAX_VIRTUAL_EVENTS 16

struct ovni_trace {
	int nlooms;
	struct ovni_loom loom[OVNI_MAX_LOOM];

	/* Index of next virtual event */
	int ivirtual;

	/* Number of virtual events stored */
	int nvirtual;

	/* The virtual events are generated by the emulator */
	struct ovni_ev *virtual_events;

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

	/* Indexed by gindex */
	struct ovni_ethread **global_thread;
	struct ovni_cpu **global_cpu;

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
	int total_nthreads;
	int total_proc;
	int total_ncpus;
};

/* Emulator function declaration */

void emu_emit(struct ovni_emu *emu);

void hook_init_ovni(struct ovni_emu *emu);
void hook_pre_ovni(struct ovni_emu *emu);
void hook_emit_ovni(struct ovni_emu *emu);
void hook_post_ovni(struct ovni_emu *emu);

void hook_init_nosv(struct ovni_emu *emu);
void hook_pre_nosv(struct ovni_emu *emu);
void hook_emit_nosv(struct ovni_emu *emu);
void hook_post_nosv(struct ovni_emu *emu);

void hook_init_tampi(struct ovni_emu *emu);
void hook_pre_tampi(struct ovni_emu *emu);

void hook_init_openmp(struct ovni_emu *emu);
void hook_pre_openmp(struct ovni_emu *emu);

struct ovni_cpu *emu_get_cpu(struct ovni_loom *loom, int cpuid);

struct ovni_ethread *emu_get_thread(struct ovni_eproc *proc, int tid);

void emu_emit_prv(struct ovni_emu *emu, int type, int val);

void
emu_virtual_ev(struct ovni_emu *emu, char *mcv);

#endif /* OVNI_EMU_H */
