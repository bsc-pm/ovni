/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef NANOS6_PRIV_H
#define NANOS6_PRIV_H

#include "emu.h"
#include "chan.h"
#include "mux.h"
#include "task.h"

/* Private enums */

enum nanos6_chan_type {
	CT_TH = 0,
	CT_CPU,
	CT_MAX
};

enum nanos6_chan {
	CH_TASKID = 0,
	CH_TYPE,
	CH_SUBSYSTEM,
	CH_RANK,
	CH_THREAD,
	CH_MAX,
};

enum nanos6_track {
	NONE = 0,
	RUN_TH,
	ACT_TH,
	TRACK_MAX,
};

extern const enum nanos6_track chan_track[CH_MAX][CT_MAX];

enum nanos6_ss_state {
	ST_TASK_BODY = 1,
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

struct nanos6_thread {
	struct chan *ch;	/* Raw, modified by nanos6 */
	struct chan *ch_run;	/* Tracking running thread */
	struct chan *ch_act;	/* Tracking active thread */
	struct chan **ch_out;	/* Output to PRV */
	struct mux *mux_run;
	struct mux *mux_act;
	struct task_stack task_stack;
};

struct nanos6_cpu {
	struct chan *ch;
	struct mux *mux;
};

struct nanos6_proc {
	struct task_info task_info;
};

int nanos6_probe(struct emu *emu);
int nanos6_create(struct emu *emu);
int nanos6_connect(struct emu *emu);
int nanos6_event(struct emu *emu);

int init_pvt(struct emu *emu);

#endif /* NANOS6_PRIV_H */
