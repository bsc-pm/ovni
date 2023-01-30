/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MODEL_NANOS6_H
#define MODEL_NANOS6_H

#include "emu_model.h"

extern struct model_spec model_nanos6;

#include "chan.h"
#include "mux.h"
#include "task.h"

enum nanos6_chan_type {
	NANOS6_CHAN_TASKID = 0,
	NANOS6_CHAN_TYPE,
	NANOS6_CHAN_SUBSYSTEM,
	NANOS6_CHAN_RANK,
	NANOS6_CHAN_THREAD,
	NANOS6_CHAN_MAX,
};

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
	struct chan chans[NANOS6_CHAN_MAX];
	struct chan fchans[NANOS6_CHAN_MAX];
	struct chan *ochans[NANOS6_CHAN_MAX];
	struct mux muxers[NANOS6_CHAN_MAX];
	struct task_stack task_stack;
};

struct nanos6_cpu {
	struct chan chans[NANOS6_CHAN_MAX];
};

struct nanos6_proc {
	struct task_info task_info;
};

#endif /* MODEL_NANOS6_H */
