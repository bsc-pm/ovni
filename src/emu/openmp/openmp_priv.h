/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef OPENMP_PRIV_H
#define OPENMP_PRIV_H

#include "emu.h"
#include "model_cpu.h"
#include "model_thread.h"

/* Private enums */

enum openmp_chan {
	CH_SUBSYSTEM = 0,
	CH_MAX,
};


enum openmp_function_values {
	ST_ATTACHED = 1,
	ST_JOIN_BARRIER,
	ST_BARRIER,
	ST_TASKING_BARRIER,
	ST_SPIN_WAIT,
	ST_FOR_STATIC,
	ST_FOR_DYNAMIC_INIT,
	ST_FOR_DYNAMIC_CHUNK,
	ST_SINGLE,
	ST_RELEASE_DEPS,
	ST_TASKWAIT_DEPS,
	ST_INVOKE_TASK,
	ST_INVOKE_TASK_IF0,
	ST_TASK_ALLOC,
	ST_TASK_SCHEDULE,
	ST_TASKWAIT,
	ST_TASKYIELD,
	ST_TASK_DUP_ALLOC,
	ST_CHECK_DEPS,
	ST_TASKGROUP,
};

struct openmp_thread {
	struct model_thread m;
};

struct openmp_cpu {
	struct model_cpu m;
};

int model_openmp_probe(struct emu *emu);
int model_openmp_create(struct emu *emu);
int model_openmp_connect(struct emu *emu);
int model_openmp_event(struct emu *emu);
int model_openmp_finish(struct emu *emu);

#endif /* OPENMP_PRIV_H */
