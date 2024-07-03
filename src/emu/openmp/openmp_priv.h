/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef OPENMP_PRIV_H
#define OPENMP_PRIV_H

#include "emu.h"
#include "task.h"
#include "model_cpu.h"
#include "model_thread.h"

/* Private enums */

enum openmp_chan {
	CH_SUBSYSTEM = 0,
	CH_LABEL,
	CH_TASKID,
	CH_MAX,
};

enum openmp_function_values {
	ST_BARRIER_FORK = 1,
	ST_BARRIER_JOIN,
	ST_BARRIER_PLAIN,
	ST_BARRIER_SPIN_WAIT,
	ST_BARRIER_TASK,
	/* Critical */
	ST_CRITICAL_ACQ,
	ST_CRITICAL_REL,
	ST_CRITICAL_SECTION,
	/* Work-distribution */
	ST_WD_DISTRIBUTE,
	ST_WD_FOR_DYNAMIC_CHUNK,
	ST_WD_FOR_DYNAMIC_INIT,
	ST_WD_FOR_STATIC,
	ST_WD_SECTION,
	ST_WD_SINGLE,
	/* Task */
	ST_TASK_ALLOC,
	ST_TASK_CHECK_DEPS,
	ST_TASK_DUP_ALLOC,
	ST_TASK_RELEASE_DEPS,
	ST_TASK_RUN,
	ST_TASK_RUN_IF0,
	ST_TASK_SCHEDULE,
	ST_TASK_TASKGROUP,
	ST_TASK_TASKWAIT,
	ST_TASK_TASKWAIT_DEPS,
	ST_TASK_TASKYIELD,
	/* Runtime */
	ST_RT_ATTACHED,
	ST_RT_FORK_CALL,
	ST_RT_INIT,
	ST_RT_MICROTASK_INTERNAL,
	ST_RT_MICROTASK_USER,
	ST_RT_WORKER_LOOP,
};

struct openmp_thread {
	struct model_thread m;
	struct task_stack task_stack;
};

struct openmp_cpu {
	struct model_cpu m;
};

struct openmp_proc {
	/* Shared among tasks and ws */
	struct task_info task_info;
};

int model_openmp_probe(struct emu *emu);
int model_openmp_create(struct emu *emu);
int model_openmp_connect(struct emu *emu);
int model_openmp_event(struct emu *emu);
int model_openmp_finish(struct emu *emu);

#endif /* OPENMP_PRIV_H */
