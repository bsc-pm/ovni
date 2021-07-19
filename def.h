#ifndef OVNI_DEF_H
#define OVNI_DEF_H

#define MAX_CPU 256
#define MAX_PROC 32
#define MAX_THR 32
#define MAX_LOOM 4
#define TRACEDIR "ovni"

/* ----------------------- common ------------------------ */

enum thread_state {
	ST_THREAD_UNINIT = 0,
	ST_THREAD_INIT = 1
};

struct __attribute__((__packed__)) event {
	uint64_t clock;
	uint8_t fsm;
	uint8_t event;
	int32_t data;
};

/* ----------------------- runtime ------------------------ */

/* State of each thread on runtime */
struct rthread {
	/* Current cpu the thread is running on. Set to -1 if unbounded or
	 * unknown */
	int cpu;

	/* Current thread id */
	pid_t tid;

	/* Clock value of the events being emitted */
	uint64_t clockvalue;

	/* Stream trace file */
	FILE *stream;

	enum thread_state state;
};

/* State of each process on runtime */
struct rproc {
	/* Path of the process tracedir */
	char dir[PATH_MAX];

	int proc;
	int loom;
	int ncpus;
	clockid_t clockid;
	char procdir[PATH_MAX];
};

/* ----------------------- emulated ------------------------ */

/* State of each thread on post-process */
struct ethread {
	/* Emulated thread tid */
	pid_t tid;

	/* Stream file */
	FILE *f;

	/* Thread stream */
	struct stream *stream;
};

/* State of each process on post-process */
struct eproc {
	/* Monotonic counter for process index */
	/* TODO: Use pid? */
	int proc;

	/* Path of the process tracedir */
	char dir[PATH_MAX];

	/* Threads */
	size_t nthreads;
	struct ethread thread[MAX_THR];
};

/* ----------------------- trace ------------------------ */

/* State of each loom on post-process */
struct loom {
	size_t nprocs;
	struct eproc proc[MAX_PROC];
};

struct stream {
	FILE *f;
	int cpu;
	int loaded;
	int active;
	struct event last;
};

struct trace {
	int nlooms;
	struct loom loom[MAX_LOOM];
	int nstreams;
	struct stream *stream;
};

#endif /* OVNI_DEF_H */
