/*
 * Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE

#include "ovni.h"
#include "compat.h"

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

static inline void
emit_ev(char *mcv)
{
	struct ovni_ev ev = { 0 };

	ovni_ev_set_mcv(&ev, mcv);
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_emit(&ev);
}

#define INSTR_3ARG(name, mcv, ta, a, tb, b, tc, c)               \
	static inline void name(ta a, tb b, tc c)                \
	{                                                        \
		struct ovni_ev ev = {0};                         \
		ovni_ev_set_mcv(&ev, mcv);                       \
		ovni_ev_set_clock(&ev, ovni_clock_now());        \
		ovni_payload_add(&ev, (uint8_t *)&a, sizeof(a)); \
		ovni_payload_add(&ev, (uint8_t *)&b, sizeof(b)); \
		ovni_payload_add(&ev, (uint8_t *)&c, sizeof(c)); \
		ovni_ev_emit(&ev);                                    \
	}

INSTR_3ARG(instr_thread_execute, "OHx", int32_t, cpu, int32_t, creator_tid, uint64_t, tag)

static inline void
instr_thread_end(void)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "OHe");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_emit(&ev);

	/* Flush the events to disk before killing the thread */
	ovni_flush();
}

static inline void
instr_start(int rank, int nranks)
{
	char hostname[HOST_NAME_MAX];

	if(gethostname(hostname, HOST_NAME_MAX) != 0)
		fail("gethostname failed");

	ovni_proc_init(1, hostname, getpid());

	ovni_proc_set_rank(rank, nranks);

	ovni_thread_init(gettid());

	/* Only the rank 0 inform about all CPUs */
	if(rank == 0)
	{
		/* Fake nranks cpus */
		for(int i=0; i < nranks; i++)
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
task_begin(int32_t id, int us)
{
	int32_t typeid = 1;
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
}

int
main(void)
{
	instr_start(0, 1);

	/* Create two nested tasks with the same task_id: this should
	 * fail */
	task_begin(1, 500);
	task_begin(1, 500);

	instr_end();

	return 0;
}

