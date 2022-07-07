/*
 * Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
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

#ifndef OVNI_TEST_NANOS6_H
#define OVNI_TEST_NANOS6_H

#include "test/common.h"
#include "compat.h"

static inline void
instr_start(int rank, int nranks)
{
	char hostname[HOST_NAME_MAX];
	char rankname[HOST_NAME_MAX + 64];

	if(gethostname(hostname, HOST_NAME_MAX) != 0)
		die("gethostname failed");

	sprintf(rankname, "%s.%d", hostname, rank);

	ovni_proc_init(1, rankname, getpid());
	ovni_proc_set_rank(rank, nranks);
	ovni_thread_init(gettid());

	/* All ranks inform CPUs */
	for(int i=0; i < nranks; i++)
		ovni_add_cpu(i, i);

	int curcpu = rank;

	dbg("thread %d has cpu %d (ncpus=%d)\n",
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

static inline void
type_create(int32_t typeid)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "6Yc");
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

INSTR_2ARG(task_create, "6Tc", int32_t, id, uint32_t, typeid)
INSTR_0ARG(task_create_end, "6TC")
INSTR_1ARG(task_execute, "6Tx", int32_t, id)

static inline void
task_begin(int32_t id, uint32_t typeid)
{
	task_create(id, typeid);
	task_create_end();
	task_execute(id);
}

INSTR_2ARG(task_block, "6Tb", int32_t, id, int32_t, reason)
INSTR_2ARG(task_unblock, "6Tu", int32_t, id, int32_t, reason)

static inline void block_enter(int32_t id)
{
	task_block(id, 0);
}

static inline void block_exit(int32_t id)
{
	task_unblock(id, 0);
}

static inline void task_wait_enter(int32_t id)
{
	task_block(id, 1);
}

static inline void task_wait_exit(int32_t id)
{
	task_unblock(id, 1);
}

static inline void waitfor_enter(int32_t id)
{
	task_block(id, 2);
}

static inline void waitfor_exit(int32_t id)
{
	task_unblock(id, 2);
}

INSTR_1ARG(task_end, "6Te", int32_t, id)

INSTR_0ARG(sched_receive_task, "6Sr")
INSTR_0ARG(sched_assign_task, "6Ss")
INSTR_0ARG(sched_self_assign_task, "6S@")
INSTR_0ARG(sched_hungry, "6Sh")
INSTR_0ARG(sched_fill, "6Sf")
INSTR_0ARG(sched_server_enter, "6S[")
INSTR_0ARG(sched_server_exit, "6S]")
INSTR_0ARG(sched_submit_enter, "6Su")
INSTR_0ARG(sched_submit_exit, "6SU")
INSTR_0ARG(enter_submit_task, "6U[")
INSTR_0ARG(exit_submit_task, "6U]")
INSTR_0ARG(register_accesses_enter, "6Dr")
INSTR_0ARG(register_accesses_exit, "6DR")
INSTR_0ARG(unregister_accesses_enter, "6Du")
INSTR_0ARG(unregister_accesses_exit, "6DU")
INSTR_0ARG(exit_create_task, "6TC")
INSTR_0ARG(spawn_function_enter, "6Hs")
INSTR_0ARG(spawn_function_exit, "6HS")

#endif /* OVNI_TEST_NANOS6_H */
