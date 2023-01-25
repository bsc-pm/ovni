/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef OVNI_MODEL_H
#define OVNI_MODEL_H

#include "ovni/ovni_model.h"
#include "chan.h"

/* The values of nanos6_ss_state are synced to the previous
 * CTF implementation. */
enum nanos6_ss_state {
	ST_NANOS6_TASK_BODY = 1,
	ST_NANOS6_TASK_CREATING,
	ST_NANOS6_TASK_SUBMIT,
	ST_NANOS6_TASK_SPAWNING,
	ST_NANOS6_TASK_FOR,
	ST_NANOS6_SCHED_ADDING,
	ST_NANOS6_SCHED_PROCESSING,
	ST_NANOS6_SCHED_SERVING,
	ST_NANOS6_DEP_REG,
	ST_NANOS6_DEP_UNREG,
	ST_NANOS6_BLK_TASKWAIT,
	ST_NANOS6_BLK_WAITFOR,
	ST_NANOS6_BLK_BLOCKING,
	ST_NANOS6_BLK_UNBLOCKING,
	ST_NANOS6_ALLOCATING,
	ST_NANOS6_FREEING,
	ST_NANOS6_HANDLING_TASK,
	ST_NANOS6_WORKER_LOOP,
	ST_NANOS6_SWITCH_TO,
	ST_NANOS6_MIGRATE,
	ST_NANOS6_SUSPEND,
	ST_NANOS6_RESUME,

	/* Value 51 is broken in old Paraver */
	EV_NANOS6_SCHED_RECV = 60,
	EV_NANOS6_SCHED_SEND,
	EV_NANOS6_SCHED_SELF,
	EV_NANOS6_CPU_IDLE,
	EV_NANOS6_CPU_ACTIVE,
	EV_NANOS6_SIGNAL,
};

enum nanos6_thread_type {
	ST_NANOS6_TH_LEADER = 1,
	ST_NANOS6_TH_MAIN = 2,
	ST_NANOS6_TH_WORKER = 3,
	ST_NANOS6_TH_EXTERNAL = 4,
};

struct nanos6_thread {
	struct task_stack nanos6_task_stack;
};

struct nanos6_proc {
	struct task_info nanos6_task_info;
};

struct nanos6_emu {
	struct ovni_emu *ovni;
};

int nanos6_model_probe(struct emu *emu);
int nanos6_model_create(struct emu *emu);
int nanos6_model_connect(struct emu *emu);
int nanos6_model_event(struct emu *emu);

#endif /* OVNI_MODEL_H */
