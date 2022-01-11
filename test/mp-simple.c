/*
 * Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
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
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void fail(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	abort();
}

static inline void emit_ev(char *mcv)
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

static inline void instr_thread_end(void)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "OHe");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_emit(&ev);

	// Flush the events to disk before killing the thread
	ovni_flush();
}

static inline void instr_start(int rank)
{
	cpu_set_t mask;
	char hostname[HOST_NAME_MAX];
	int i, j = 0, curcpu = -1;
	int last_phy;

	if(gethostname(hostname, HOST_NAME_MAX) != 0)
		fail("gethostname failed");

	if(sched_getaffinity(0, sizeof(mask), &mask) < 0)
		fail("sched_getaffinity failed");

	ovni_proc_init(1, hostname, getpid());
	ovni_thread_init(gettid());

	/* Only the rank 0 inform about all CPUs */
	if(rank == 0)
	{
		for(i=0; i < CPU_SETSIZE; i++)
		{
			if(CPU_ISSET(i, &mask))
			{
				last_phy = i;

				if (rank == 0)
					ovni_add_cpu(j++, i);
			}
		}
	}

	if(CPU_COUNT(&mask) == 1)
		curcpu = last_phy;
	else
		curcpu = -1;

	fprintf(stderr, "thread %d has cpu %d (ncpus=%d)\n",
			gettid(), curcpu, CPU_COUNT(&mask));

	instr_thread_execute(curcpu, -1, 0);
}

static inline void instr_end(void)
{
	instr_thread_end();
	ovni_thread_free();
	ovni_proc_fini();
}

int main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	//int nranks = atoi(getenv("OVNI_NRANKS"));

	instr_start(rank);

	usleep(50 * 1000);

	instr_end();

	return 0;
}

