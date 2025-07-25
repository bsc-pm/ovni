/* Copyright (c) 2021-2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef INSTR_NOSV_H
#define INSTR_NOSV_H

#include "instr.h"

#include "task.h"

static inline void
instr_nosv_init(void)
{
	instr_require("nosv");
	ovni_attr_set_boolean("nosv.can_breakdown", 1);
}

static inline uint32_t
instr_nosv_type_create(int32_t typeid)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "VYc");
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

static inline void
instr_nosv_hwc(size_t n, int64_t *counters)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "VWC");
	ovni_ev_set_clock(&ev, (uint64_t) get_clock());

	uint32_t nbytes = (uint32_t) (n * sizeof(int64_t));
	ovni_ev_jumbo_emit(&ev, (uint8_t *) counters, (uint32_t) nbytes);
}

INSTR_2ARG(instr_nosv_task_create,     "VTc", uint32_t, taskid, uint32_t, typeid)
INSTR_2ARG(instr_nosv_task_create_par, "VTC", uint32_t, taskid, uint32_t, typeid)
INSTR_2ARG(instr_nosv_task_execute,    "VTx", uint32_t, taskid, uint32_t, bodyid)
INSTR_2ARG(instr_nosv_task_pause,      "VTp", uint32_t, taskid, uint32_t, bodyid)
INSTR_2ARG(instr_nosv_task_resume,     "VTr", uint32_t, taskid, uint32_t, bodyid)
INSTR_2ARG(instr_nosv_task_end,        "VTe", uint32_t, taskid, uint32_t, bodyid)

INSTR_0ARG(instr_nosv_submit_enter, "VAs")
INSTR_0ARG(instr_nosv_submit_exit,  "VAS")
INSTR_0ARG(instr_nosv_attach_enter, "VAa")
INSTR_0ARG(instr_nosv_attach_exit,  "VAA")
INSTR_0ARG(instr_nosv_detach_enter, "VAe")
INSTR_0ARG(instr_nosv_detach_exit,  "VAE")
INSTR_0ARG(instr_nosv_barrier_wait_enter,  "VAb")
INSTR_0ARG(instr_nosv_barrier_wait_exit,   "VAB")
INSTR_0ARG(instr_nosv_mutex_lock_enter,    "VAl")
INSTR_0ARG(instr_nosv_mutex_lock_exit,     "VAL")
INSTR_0ARG(instr_nosv_mutex_trylock_enter, "VAt")
INSTR_0ARG(instr_nosv_mutex_trylock_exit,  "VAT")
INSTR_0ARG(instr_nosv_mutex_unlock_enter,  "VAu")
INSTR_0ARG(instr_nosv_mutex_unlock_exit,   "VAU")
INSTR_0ARG(instr_nosv_cond_wait_enter,      "VAo")
INSTR_0ARG(instr_nosv_cond_wait_exit,       "VAO")
INSTR_0ARG(instr_nosv_cond_signal_enter,    "VAg")
INSTR_0ARG(instr_nosv_cond_signal_exit,     "VAG")
INSTR_0ARG(instr_nosv_cond_broadcast_enter, "VAk")
INSTR_0ARG(instr_nosv_cond_broadcast_exit,  "VAK")
INSTR_0ARG(instr_nosv_attached, "VHa") /* deprecated */
INSTR_0ARG(instr_nosv_detached, "VHA") /* deprecated */


#endif /* INSTR_NOSV_H */
