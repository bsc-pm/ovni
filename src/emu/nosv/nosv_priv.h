/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef NOSV_PRIV_H
#define NOSV_PRIV_H

#include "emu.h"
#include "task.h"
#include "model_cpu.h"
#include "model_thread.h"

/* Private enums */

enum nosv_chan {
	CH_TASKID = 0,
	CH_TYPE,
	CH_APPID,
	CH_SUBSYSTEM,
	CH_RANK,
	CH_MAX,
};

enum nosv_ss_values {
	ST_SCHED_HUNGRY = 6,
	ST_SCHED_SERVING,
	ST_SCHED_SUBMITTING,
	ST_MEM_ALLOCATING,
	ST_MEM_FREEING,
	ST_TASK_BODY,
	ST_API_CREATE,
	ST_API_DESTROY,
	ST_API_SUBMIT,
	ST_API_PAUSE,
	ST_API_YIELD,
	ST_API_WAITFOR,
	ST_API_SCHEDPOINT,
	ST_ATTACH,
	ST_WORKER,
	ST_DELEGATE,

	EV_SCHED_RECV,
	EV_SCHED_SEND,
	EV_SCHED_SELF,
};

struct nosv_thread {
	struct model_thread m;
	struct task_stack task_stack;
};

struct nosv_cpu {
	struct model_cpu m;
};

struct nosv_proc {
	struct task_info task_info;
};

int model_nosv_probe(struct emu *emu);
int model_nosv_create(struct emu *emu);
int model_nosv_connect(struct emu *emu);
int model_nosv_event(struct emu *emu);
int model_nosv_finish(struct emu *emu);

#endif /* NOSV_PRIV_H */
