#ifndef OVNI_EMU_H
#define OVNI_EMU_H

#include "ovni.h"
#include "uthash.h"
#include "parson.h"
#include "heap.h"
#include <stdio.h>

/* Debug macros */

#ifdef ENABLE_DEBUG
# define dbg(...) fprintf(stderr, __VA_ARGS__);
#else
# define dbg(...)
#endif

#define err(...) fprintf(stderr, __VA_ARGS__);

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define UNUSED(x) (void)(x)

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

enum error_values {
	ST_BAD = 666,
	ST_TOO_MANY_TH = 777,
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

	CHAN_NOSV_TASKID,
	CHAN_NOSV_TYPEID,
	CHAN_NOSV_APPID,
	CHAN_NOSV_SUBSYSTEM,

	CHAN_TAMPI_MODE,
	CHAN_OPENMP_MODE,

	CHAN_MAX
};

enum chan_to_prv_type {
	CHAN_ID = 0,
	CHAN_TH = 1,
	CHAN_CPU = 2,
};

/* Same order as `enum chan` */
static const int chan_to_prvtype[CHAN_MAX][3] = {
	/* Channel		TH  CPU */
	{ CHAN_OVNI_PID,	10, 60 },
	{ CHAN_OVNI_TID,	11, 61 },
	{ CHAN_OVNI_NRTHREADS,	-1, 62 },
	{ CHAN_OVNI_STATE,	13, -1 },
	{ CHAN_OVNI_APPID,	14, 64 }, /* Not used */
	{ CHAN_OVNI_CPU,	15, -1 },

	{ CHAN_NOSV_TASKID,	20, 70 },
	{ CHAN_NOSV_TYPEID,	21, 71 },
	{ CHAN_NOSV_APPID,	22, 72 },
	{ CHAN_NOSV_SUBSYSTEM,	23, 73 },

	{ CHAN_TAMPI_MODE,	30, 80 },

	{ CHAN_OPENMP_MODE,	40, 90 },
};

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

	/* Stream fd */
	int stream_fd;

	enum ethread_state state;
	int is_running;
	int is_active;

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

	/* The loom of the CPU */
	struct ovni_loom *loom;

	/* CPU channels */
	struct ovni_chan chan[CHAN_MAX];

	/* The threads assigned to this CPU */
	size_t nthreads;
	struct ovni_ethread *thread[OVNI_MAX_THR];

	/* Running threads */
	size_t nrunning_threads;
	struct ovni_ethread *th_running;

	/* Active threads (not paused or dead) */
	size_t nactive_threads;
	struct ovni_ethread *th_active;

	/* Cpu name as shown in paraver row */
	char name[MAX_CPU_NAME];
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
	size_t nlooms;
	struct ovni_loom loom[OVNI_MAX_LOOM];

	/* Index of next virtual event */
	size_t ivirtual;

	/* Number of virtual events stored */
	size_t nvirtual;

	/* The virtual events are generated by the emulator */
	struct ovni_ev *virtual_events;

	size_t nstreams;
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

	struct nosv_task *cur_task;

	/* Global processed size and offset of all streams */
	size_t global_size;
	size_t global_offset;
	double start_emulation_time;

	int64_t firstclock;
	int64_t lastclock;
	int64_t delta_time;

	/* Counters for statistics */
	int64_t nev_processed;

	FILE *prv_thread;
	FILE *prv_cpu;
	FILE *pcf_thread;
	FILE *pcf_cpu;

	char *clock_offset_file;
	char *tracedir;

	/* Total counters */
	size_t total_nthreads;
	size_t total_proc;
	size_t total_ncpus;

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

void hook_init_tampi(struct ovni_emu *emu);
void hook_pre_tampi(struct ovni_emu *emu);

void hook_init_openmp(struct ovni_emu *emu);
void hook_pre_openmp(struct ovni_emu *emu);

struct ovni_cpu *emu_get_cpu(struct ovni_loom *loom, int cpuid);

struct ovni_ethread *emu_get_thread(struct ovni_eproc *proc, int tid);

void emu_cpu_update_chan(struct ovni_cpu *cpu, struct ovni_chan *cpu_chan);

void
emu_virtual_ev(struct ovni_emu *emu, char *mcv);

#endif /* OVNI_EMU_H */
