/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef INSTR_OPENMP_H
#define INSTR_OPENMP_H

#include "instr.h"

#include "task.h"

static inline void
instr_openmp_init(void)
{
	instr_require("openmp");
}

static inline uint32_t
instr_openmp_type_create(uint32_t typeid, const char *label)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "POc");
	ovni_ev_set_clock(&ev, (uint64_t) get_clock());

	char buf[256];
	char *p = buf;

	size_t nbytes = 0;
	memcpy(buf, &typeid, sizeof(typeid));
	p += sizeof(typeid);
	nbytes += sizeof(typeid);
	sprintf(p, "%s.%d", label, typeid);
	nbytes += strlen(p) + 1;

	ovni_ev_jumbo_emit(&ev, (uint8_t *) buf, (uint32_t) nbytes);

	return task_get_type_gid(p);
}

INSTR_2ARG(instr_openmp_task_create,  "PPc", uint32_t, taskid, uint32_t, typeid)
INSTR_1ARG(instr_openmp_task_execute, "PPx", uint32_t, taskid)
INSTR_1ARG(instr_openmp_task_end,     "PPe", uint32_t, taskid)

INSTR_1ARG(instr_openmp_ws_enter, "PQx", uint32_t, typeid)
INSTR_1ARG(instr_openmp_ws_exit,  "PQe", uint32_t, typeid)

#endif /* INSTR_OPENMP_H */
