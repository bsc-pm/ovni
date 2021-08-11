#ifndef OVNI_H
#define OVNI_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <linux/limits.h>
#include <limits.h>

#include "parson.h"

#define OVNI_MAX_CPU 256
#define OVNI_MAX_PROC 256
#define OVNI_MAX_THR 256
#define OVNI_MAX_LOOM 4
#define OVNI_TRACEDIR "ovni"
#define OVNI_MAX_HOSTNAME 512

/* Reserved buffer for event allocation per thread */
#define OVNI_MAX_EV_BUF (2 * 1024LL * 1024LL) /* 2 MiB */

/* ----------------------- common ------------------------ */

enum ovni_ev_flags {
	OVNI_EV_JUMBO = 0x10,
};

struct __attribute__((__packed__)) ovni_jumbo_payload {
	uint32_t size;
	uint8_t data[1];
};

union __attribute__((__packed__)) ovni_ev_payload {

	int8_t i8[16];
	int16_t i16[8];
	int32_t i32[4];
	int64_t i64[2];

	uint8_t u8[16];
	uint16_t u16[8];
	uint32_t u32[4];
	uint64_t u64[2];

	struct ovni_jumbo_payload jumbo;
};

struct __attribute__((__packed__)) ovni_ev_header {
	/* first 4 bits reserved, last 4 for payload size */
	uint8_t flags;
	uint8_t model;
	uint8_t class;
	uint8_t value;
	uint64_t clock;
};

struct __attribute__((__packed__)) ovni_ev {
	struct ovni_ev_header header;

	/* The payload size may vary depending on the ev type:
	 *   - normal: 0, or 2 to 16 bytes
	 *   - jumbo: 0 to 2^32 - 1 bytes */
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

	int app;
	int proc;
	char loom[OVNI_MAX_HOSTNAME];
	int ncpus;
	clockid_t clockid;
	char procdir[PATH_MAX];

	int ready;

	JSON_Value *meta;
};

int ovni_proc_init(int app, char *loom, int proc);

int ovni_proc_fini();

int ovni_thread_init(pid_t tid);

int ovni_thread_isready();

void ovni_clock_update();

void ovni_ev_set_mcv(struct ovni_ev *ev, char *mcv);

uint64_t ovni_ev_get_clock(struct ovni_ev *ev);

void ovni_payload_add(struct ovni_ev *ev, uint8_t *buf, int size);

int ovni_ev_size(struct ovni_ev *ev);

int ovni_payload_size(struct ovni_ev *ev);

void ovni_add_cpu(int index, int phyid);

/* Set the current clock in the event and queue it */
void ovni_ev(struct ovni_ev *ev);
void ovni_ev_jumbo(struct ovni_ev *ev, uint8_t *buf, uint32_t bufsize);

int ovni_flush();


#endif /* OVNI_H */
