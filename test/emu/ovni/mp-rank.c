/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _GNU_SOURCE

#include "compat.h"
#include "ovni.h"

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void
fail(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	abort();
}

#define INSTR_3ARG(name, mcv, ta, a, tb, b, tc, c)                \
	static inline void name(ta a, tb b, tc c)                 \
	{                                                         \
		struct ovni_ev ev = {0};                          \
		ovni_ev_set_mcv(&ev, mcv);                        \
		ovni_ev_set_clock(&ev, ovni_clock_now());         \
		ovni_payload_add(&ev, (uint8_t *) &a, sizeof(a)); \
		ovni_payload_add(&ev, (uint8_t *) &b, sizeof(b)); \
		ovni_payload_add(&ev, (uint8_t *) &c, sizeof(c)); \
		ovni_ev_emit(&ev);                                \
	}

INSTR_3ARG(instr_thread_execute, "OHx", int32_t, cpu, int32_t, creator_tid, uint64_t, tag)

static inline void
instr_thread_end(void)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "OHe");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_emit(&ev);

	// Flush the events to disk before killing the thread
	ovni_flush();
}

static inline void
instr_start(int rank, int nranks)
{
	char hostname[HOST_NAME_MAX];

	if (gethostname(hostname, HOST_NAME_MAX) != 0)
		fail("gethostname failed");

	ovni_proc_init(1, hostname, getpid());

	ovni_proc_set_rank(rank, nranks);

	ovni_thread_init(gettid());

	/* Only the rank 0 inform about all CPUs */
	if (rank == 0) {
		/* Fake nranks cpus */
		for (int i = 0; i < nranks; i++)
			ovni_add_cpu(i, i);
	}

	int curcpu = rank;

	fprintf(stderr, "thread %d has cpu %d (ncpus=%d)\n",
			gettid(), curcpu, nranks);

	instr_thread_execute(curcpu, -1, 0);
}

static inline void
instr_end(void)
{
	instr_thread_end();
	ovni_thread_free();
	ovni_proc_fini();
}

static void
type_create(int32_t typeid)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "VYc");
	ovni_ev_set_clock(&ev, ovni_clock_now());

	char buf[256];
	char *p = buf;

	size_t nbytes = 0;
	memcpy(buf, &typeid, sizeof(typeid));
	p += sizeof(typeid);
	nbytes += sizeof(typeid);
	sprintf(p, "testtype%d", typeid);
	nbytes += strlen(p) + 1;

	ovni_ev_jumbo_emit(&ev, (uint8_t *) buf, nbytes);
}

static void
task(int32_t id, uint32_t typeid, int us)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "VTc");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_payload_add(&ev, (uint8_t *) &id, sizeof(id));
	ovni_payload_add(&ev, (uint8_t *) &typeid, sizeof(id));
	ovni_ev_emit(&ev);

	memset(&ev, 0, sizeof(ev));

	ovni_ev_set_mcv(&ev, "VTx");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_payload_add(&ev, (uint8_t *) &id, sizeof(id));
	ovni_ev_emit(&ev);

	usleep(us);

	memset(&ev, 0, sizeof(ev));

	ovni_ev_set_mcv(&ev, "VTe");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_payload_add(&ev, (uint8_t *) &id, sizeof(id));
	ovni_ev_emit(&ev);
}

int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));
	uint32_t typeid = 1;

	instr_start(rank, nranks);

	type_create(typeid);

	/* Create some fake nosv tasks */
	for (int i = 0; i < 10; i++)
		task(i + 1, typeid, 5000);

	instr_end();

	return 0;
}
