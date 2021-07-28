#ifndef OVNI_H
#define OVNI_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>

#define OVNI_MAX_CPU 256
#define OVNI_MAX_PROC 32
#define OVNI_MAX_THR 32
#define OVNI_MAX_LOOM 4
#define OVNI_TRACEDIR "ovni"

/* Reserved buffer for event allocation */
#define OVNI_MAX_EV_BUF (16 * 1024LL * 1024LL * 1024LL)

/* ----------------------- common ------------------------ */

union __attribute__((__packed__)) ovni_ev_payload {
	uint8_t u8[16];
	int8_t i8[16];
	uint16_t u16[8];
	int16_t i16[8];
	uint32_t u32[4];
	int32_t i32[4];
	uint64_t u64[2];
	int64_t i64[2];
};

struct __attribute__((__packed__)) ovni_ev {
	/* first 4 bits reserved, last 4 for payload size */
	uint8_t flags;
	uint8_t model;
	uint8_t class;
	uint8_t value;
	uint16_t clock_hi;
	uint32_t clock_lo;
	union ovni_ev_payload payload;
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


#endif /* OVNI_H */
