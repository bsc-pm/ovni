/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef INSTR_NANOS6_H
#define INSTR_NANOS6_H

#include "instr.h"

#include "task.h"

static inline void
instr_nanos6_init(void)
{
	instr_require("nanos6");
}

static inline uint32_t
instr_nanos6_type_create(int32_t typeid)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "6Yc");
	ovni_ev_set_clock(&ev, (uint64_t) get_clock());

	char buf[256];
	char *p = buf;

	size_t nbytes = 0;
	memcpy(buf, &typeid, sizeof(typeid));
	p += sizeof(typeid);
	nbytes += sizeof(typeid);
	sprintf(p, "testtype%d", typeid);
	nbytes += strlen(p) + 1;

	ovni_ev_jumbo_emit(&ev, (uint8_t *) buf, (uint32_t) nbytes);

	return task_get_type_gid(p);
}

INSTR_2ARG(instr_nanos6_task_create, "6Tc", int32_t, id, uint32_t, typeid)
INSTR_1ARG(instr_nanos6_task_execute, "6Tx", int32_t, id)
INSTR_1ARG(instr_nanos6_task_pause, "6Tp", int32_t, id)
INSTR_1ARG(instr_nanos6_task_resume, "6Tr", int32_t, id)
INSTR_1ARG(instr_nanos6_task_end, "6Te", int32_t, id)

INSTR_0ARG(instr_nanos6_task_create_begin, "6C[")
INSTR_0ARG(instr_nanos6_task_create_end, "6C]")

INSTR_0ARG(instr_nanos6_sched_server_enter, "6S[")
INSTR_0ARG(instr_nanos6_sched_server_exit, "6S]")
INSTR_0ARG(instr_nanos6_add_ready_enter, "6Sa")
INSTR_0ARG(instr_nanos6_add_ready_exit, "6SA")
INSTR_0ARG(instr_nanos6_process_ready_enter, "6Sp")
INSTR_0ARG(instr_nanos6_process_ready_exit, "6SP")
INSTR_0ARG(instr_nanos6_sched_receive, "6Sr")
INSTR_0ARG(instr_nanos6_sched_assign, "6Ss")
INSTR_0ARG(instr_nanos6_sched_self_assign, "6S@")

INSTR_0ARG(instr_nanos6_worker_loop_enter, "6W[")
INSTR_0ARG(instr_nanos6_worker_loop_exit, "6W]")
INSTR_0ARG(instr_nanos6_handle_task_enter, "6Wt")
INSTR_0ARG(instr_nanos6_handle_task_exit, "6WT")
INSTR_0ARG(instr_nanos6_switch_to_enter, "6Ww")
INSTR_0ARG(instr_nanos6_switch_to_exit, "6WW")
INSTR_0ARG(instr_nanos6_migrate_enter, "6Wm")
INSTR_0ARG(instr_nanos6_migrate_exit, "6WM")
INSTR_0ARG(instr_nanos6_suspend_enter, "6Ws")
INSTR_0ARG(instr_nanos6_suspend_exit, "6WS")
INSTR_0ARG(instr_nanos6_resume_enter, "6Wr")
INSTR_0ARG(instr_nanos6_resume_exit, "6WR")
INSTR_0ARG(instr_nanos6_sponge_enter, "6Wg")
INSTR_0ARG(instr_nanos6_sponge_exit, "6WG")
INSTR_0ARG(instr_nanos6_signal, "6W*")

INSTR_0ARG(instr_nanos6_submit_task_enter, "6U[")
INSTR_0ARG(instr_nanos6_submit_task_exit, "6U]")

INSTR_0ARG(instr_nanos6_spawn_function_enter, "6F[")
INSTR_0ARG(instr_nanos6_spawn_function_exit, "6F]")

INSTR_0ARG(instr_nanos6_taskfor_start, "6O[")
INSTR_0ARG(instr_nanos6_taskfor_stop, "6O]")

INSTR_0ARG(instr_nanos6_task_body_enter, "6t[")
INSTR_0ARG(instr_nanos6_task_body_exit, "6t]")

INSTR_0ARG(instr_nanos6_register_enter, "6Dr")
INSTR_0ARG(instr_nanos6_register_exit, "6DR")
INSTR_0ARG(instr_nanos6_unregister_enter, "6Du")
INSTR_0ARG(instr_nanos6_unregister_exit, "6DU")

INSTR_0ARG(instr_nanos6_block_enter, "6Bb")
INSTR_0ARG(instr_nanos6_block_exit, "6BB")
INSTR_0ARG(instr_nanos6_unblock_enter, "6Bb")
INSTR_0ARG(instr_nanos6_unblock_exit, "6BB")
INSTR_0ARG(instr_nanos6_taskwait_enter, "6Bw")
INSTR_0ARG(instr_nanos6_taskwait_exit, "6BW")
INSTR_0ARG(instr_nanos6_waitfor_enter, "6Bf")
INSTR_0ARG(instr_nanos6_waitfor_exit, "6BF")

INSTR_0ARG(instr_nanos6_external_set, "6He")
INSTR_0ARG(instr_nanos6_external_unset, "6HE")
INSTR_0ARG(instr_nanos6_worker_set, "6Hw")
INSTR_0ARG(instr_nanos6_worker_unset, "6HW")
INSTR_0ARG(instr_nanos6_leader_set, "6Hl")
INSTR_0ARG(instr_nanos6_leader_unset, "6HL")
INSTR_0ARG(instr_nanos6_main_set, "6Hm")
INSTR_0ARG(instr_nanos6_main_unset, "6HM")

INSTR_0ARG(instr_nanos6_alloc_enter, "6Ma")
INSTR_0ARG(instr_nanos6_alloc_exit, "6MA")
INSTR_0ARG(instr_nanos6_free_enter, "6Mf")
INSTR_0ARG(instr_nanos6_free_exit, "6MF")

INSTR_0ARG(instr_nanos6_progressing, "6Pp")
INSTR_0ARG(instr_nanos6_resting,     "6Pr")
INSTR_0ARG(instr_nanos6_absorbing,   "6Pa")

static inline void
instr_nanos6_task_create_and_execute(int32_t id, uint32_t typeid)
{
	instr_nanos6_task_create_begin();
	instr_nanos6_task_create(id, typeid);
	instr_nanos6_task_create_end();
	instr_nanos6_task_body_enter();
	instr_nanos6_task_execute(id);
}

#endif /* INSTR_NANOS6_H */
