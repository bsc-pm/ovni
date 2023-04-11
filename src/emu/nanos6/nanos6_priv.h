/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef NANOS6_PRIV_H
#define NANOS6_PRIV_H

#include "emu.h"
#include "task.h"
#include "sort.h"
#include "model_cpu.h"
#include "model_thread.h"
#include "breakdown.h"
#include "pv/pcf.h"

/* Private enums */

enum nanos6_chan {
	CH_TASKID = 0,
	CH_TYPE,
	CH_SUBSYSTEM,
	CH_RANK,
	CH_THREAD,
	CH_IDLE,
	CH_MAX,
};

/* Same order as shown in breakdown view */
enum nanos6_ss_state {
	ST_TASK_BODY = 1,
	ST_UNKNOWN_SS,
	ST_TASK_CREATING,
	ST_TASK_SUBMIT,
	ST_TASK_SPAWNING,
	ST_TASK_FOR,
	ST_SCHED_ADDING,
	ST_SCHED_PROCESSING,
	ST_SCHED_SERVING,
	ST_DEP_REG,
	ST_DEP_UNREG,
	ST_BLK_TASKWAIT,
	ST_BLK_WAITFOR,
	ST_BLK_BLOCKING,
	ST_BLK_UNBLOCKING,
	ST_ALLOCATING,
	ST_FREEING,
	ST_HANDLING_TASK,
	ST_WORKER_LOOP,
	ST_SWITCH_TO,
	ST_MIGRATE,
	ST_SUSPEND,
	ST_RESUME,
	ST_SPONGE,

	/* Value 51 is broken in old Paraver */
	EV_SCHED_RECV = 60,
	EV_SCHED_SEND,
	EV_SCHED_SELF,
	EV_CPU_IDLE,
	EV_CPU_ACTIVE,
	EV_SIGNAL,
};

enum nanos6_thread_type {
	ST_TH_LEADER = 1,
	ST_TH_MAIN = 2,
	ST_TH_WORKER = 3,
	ST_TH_EXTERNAL = 4,
};

enum nanos6_progress {
	/* Can mix with subsystem values */
	ST_PROGRESSING = 100,
	ST_RESTING,
	ST_ABSORBING,
};

struct nanos6_thread {
	struct model_thread m;
	struct task_stack task_stack;
};

struct nanos6_cpu {
	struct model_cpu m;
	struct breakdown_cpu breakdown;
};

struct nanos6_proc {
	struct task_info task_info;
};

struct nanos6_emu {
	int connected;
	int event;
	struct breakdown_emu breakdown;
};

int model_nanos6_probe(struct emu *emu);
int model_nanos6_create(struct emu *emu);
int model_nanos6_connect(struct emu *emu);
int model_nanos6_event(struct emu *emu);
int model_nanos6_finish(struct emu *emu);

int model_nanos6_breakdown_create(struct emu *emu);
int model_nanos6_breakdown_connect(struct emu *emu);
int model_nanos6_breakdown_finish(struct emu *emu,
		const struct pcf_value_label **labels);

#endif /* NANOS6_PRIV_H */
