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

#ifndef INSTR_NANOS6_H
#define INSTR_NANOS6_H

#include "../instr.h"
#include "compat.h"

static inline void
instr_nanos6_type_create(int32_t typeid)
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

INSTR_2ARG(instr_nanos6_task_create, "6Tc", int32_t, id, uint32_t, typeid)
INSTR_0ARG(instr_nanos6_task_create_end, "6TC")
INSTR_1ARG(instr_nanos6_task_execute, "6Tx", int32_t, id)
INSTR_1ARG(instr_nanos6_task_pause, "6Tp", int32_t, id)
INSTR_1ARG(instr_nanos6_task_resume, "6Tr", int32_t, id)
INSTR_1ARG(instr_nanos6_task_end, "6Te", int32_t, id)

static inline void
instr_nanos6_task_create_and_execute(int32_t id, uint32_t typeid)
{
	instr_nanos6_task_create(id, typeid);
	instr_nanos6_task_create_end();
	instr_nanos6_task_execute(id);
}

INSTR_0ARG(instr_nanos6_block_enter, "6Bb")
INSTR_0ARG(instr_nanos6_block_exit, "6BB")
INSTR_0ARG(instr_nanos6_unblock_enter, "6Bb")
INSTR_0ARG(instr_nanos6_unblock_exit, "6BB")
INSTR_0ARG(instr_nanos6_taskwait_enter, "6Bw")
INSTR_0ARG(instr_nanos6_taskwait_exit, "6BW")
INSTR_0ARG(instr_nanos6_waitfor_enter, "6Bf")
INSTR_0ARG(instr_nanos6_waitfor_exit, "6BF")

INSTR_0ARG(instr_nanos6_sched_receive_task, "6Sr")
INSTR_0ARG(instr_nanos6_sched_assign_task, "6Ss")
INSTR_0ARG(instr_nanos6_sched_self_assign_task, "6S@")
INSTR_0ARG(instr_nanos6_sched_hungry, "6Sh")
INSTR_0ARG(instr_nanos6_sched_fill, "6Sf")
INSTR_0ARG(instr_nanos6_sched_server_enter, "6S[")
INSTR_0ARG(instr_nanos6_sched_server_exit, "6S]")
INSTR_0ARG(instr_nanos6_sched_submit_enter, "6Su")
INSTR_0ARG(instr_nanos6_sched_submit_exit, "6SU")
INSTR_0ARG(instr_nanos6_enter_submit_task, "6U[")
INSTR_0ARG(instr_nanos6_exit_submit_task, "6U]")
INSTR_0ARG(instr_nanos6_register_accesses_enter, "6Dr")
INSTR_0ARG(instr_nanos6_register_accesses_exit, "6DR")
INSTR_0ARG(instr_nanos6_unregister_accesses_enter, "6Du")
INSTR_0ARG(instr_nanos6_unregister_accesses_exit, "6DU")
INSTR_0ARG(instr_nanos6_exit_create_task, "6TC")
INSTR_0ARG(instr_nanos6_spawn_function_enter, "6Hs")
INSTR_0ARG(instr_nanos6_spawn_function_exit, "6HS")

#endif /* INSTR_NANOS6_H */
