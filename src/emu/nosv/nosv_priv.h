/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef NOSV_PRIV_H
#define NOSV_PRIV_H

#include "breakdown.h"
#include "emu.h"
#include "task.h"
#include "model_cpu.h"
#include "model_thread.h"

/* Private enums */

enum nosv_chan {
	CH_BODYID = 0,
	CH_TASKID,
	CH_TYPE,
	CH_APPID,
	CH_SUBSYSTEM,
	CH_RANK,
	CH_IDLE,
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
	ST_API_ATTACH,
	ST_API_DETACH,
	ST_API_MUTEX_LOCK,
	ST_API_MUTEX_TRYLOCK,
	ST_API_MUTEX_UNLOCK,
	ST_API_BARRIER_WAIT,
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
	struct nosv_breakdown_cpu breakdown;
};

struct nosv_proc {
	struct task_info task_info;
};

struct nosv_emu {
	int connected;
	int event;
	struct nosv_breakdown_emu breakdown;
};

enum nosv_progress {
	/* Can mix with subsystem values */
	ST_PROGRESSING = 100,
	ST_RESTING,
	ST_ABSORBING,
};

int model_nosv_probe(struct emu *emu);
int model_nosv_create(struct emu *emu);
int model_nosv_connect(struct emu *emu);
int model_nosv_event(struct emu *emu);
int model_nosv_finish(struct emu *emu);

int model_nosv_breakdown_create(struct emu *emu);
int model_nosv_breakdown_connect(struct emu *emu);
int model_nosv_breakdown_finish(struct emu *emu,
		const struct pcf_value_label **labels);


#endif /* NOSV_PRIV_H */
