#ifndef OVNI_H
#define OVNI_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>

#include "ovni.h"

#define OVNI_MAX_CPU 256
#define OVNI_MAX_PROC 32
#define OVNI_MAX_THR 32
#define OVNI_MAX_LOOM 4
#define OVNI_TRACEDIR "ovni"

/* Reserved buffer for event allocation */
#define OVNI_MAX_EV_BUF (16 * 1024LL * 1024LL * 1024LL)

/* ----------------------- common ------------------------ */

struct __attribute__((__packed__)) ovni_ev {
	/* first 4 bits reserved, last 4 for payload size */
	uint8_t flags;
	uint8_t model;
	uint8_t class;
	uint8_t value;
	uint16_t clock_hi;
	uint32_t clock_lo;
	union {
		uint8_t payload_u8[16];
		uint16_t payload_u16[8];
		uint32_t payload_u32[4];
		uint64_t payload_u64[2];
	} payload;
};

/* ----------------------- runtime ------------------------ */

/* State of each thread on runtime */
struct ovni_rthread {
	/* Current cpu the thread is running on. Set to -1 if unbounded or
	 * unknown */
	int cpu;

	/* Current thread id */
	pid_t tid;

	/* Clock value of the events being emitted */
	uint64_t clockvalue;

	/* Stream trace file descriptor */
	int streamfd;

	int ready;

	/* The number of bytes filled with events */
	size_t evlen;

	/* Buffer to write events */
	uint8_t *evbuf;
};

/* State of each process on runtime */
struct ovni_rproc {
	/* Path of the process tracedir */
	char dir[PATH_MAX];

	int proc;
	int loom;
	int ncpus;
	clockid_t clockid;
	char procdir[PATH_MAX];

	int ready;
};

/* ----------------------- emulated ------------------------ */

struct ovni_cpu;

/* State of each thread on post-process */
struct ovni_ethread {
	/* Emulated thread tid */
	pid_t tid;

	/* Stream file */
	FILE *f;

	/* Thread stream */
	struct ovni_stream *stream;

	/* Current cpu */
	struct ovni_cpu *cpu;
};

/* State of each process on post-process */
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

struct ovni_cpu {
	/* The thread the cpu is currently running */
	struct ovni_ethread *thread;
};

struct ovni_evhead {
	struct ovni_stream *stream;

	struct ovni_loom *loom;
	struct ovni_eproc *proc;
	struct ovni_ethread *thread;
};

struct ovni_emulator {
	struct ovni_trace trace;
	struct ovni_cpu cpu[OVNI_MAX_CPU];

	struct ovni_evhead head;
	uint64_t lastclock;
};







int ovni_proc_init(int loom, int proc);

int ovni_thread_init(pid_t tid);

int ovni_thread_isready();

void ovni_clock_update();

void ovni_ev_set_mcv(struct ovni_ev *ev, char *mcv);

uint64_t ovni_ev_get_clock(struct ovni_ev *ev);

void ovni_payload_add(struct ovni_ev *ev, uint8_t *buf, int size);

int ovni_ev_size(struct ovni_ev *ev);

int ovni_payload_size(struct ovni_ev *ev);

/* Set the current clock in the event and queue it */
void ovni_ev(struct ovni_ev *ev);

int ovni_flush();

int ovni_load_trace(struct ovni_trace *trace, char *tracedir);

int ovni_load_streams(struct ovni_trace *trace);

void ovni_free_streams(struct ovni_trace *trace);

void ovni_load_next_event(struct ovni_stream *stream);

#endif /* OVNI_H */
