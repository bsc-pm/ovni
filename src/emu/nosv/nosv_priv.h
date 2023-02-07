/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef NOSV_PRIV_H
#define NOSV_PRIV_H

#include "emu.h"
#include "chan.h"
#include "mux.h"
#include "task.h"

/* Private enums */

enum nosv_chan_type {
	CT_TH = 0,
	CT_CPU,
	CT_MAX
};

enum nosv_chan {
	CH_TASKID = 0,
	CH_TYPE,
	CH_APPID,
	CH_SUBSYSTEM,
	CH_RANK,
	CH_MAX,
};

enum nosv_track {
	NONE = 0,
	RUN_TH,
	ACT_TH,
	TRACK_MAX,
};

extern const enum nosv_track nosv_chan_track[CH_MAX][CT_MAX];

enum nosv_ss_values {
	ST_SCHED_HUNGRY = 6,
	ST_SCHED_SERVING,
	ST_SCHED_SUBMITTING,
	ST_MEM_ALLOCATING,
	ST_MEM_FREEING,
	ST_TASK_RUNNING,
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
	struct chan *ch;	/* Raw, modified by nosv */
	struct chan *ch_run;	/* Tracking running thread */
	struct chan *ch_act;	/* Tracking active thread */
	struct chan **ch_out;	/* Output to PRV */
	struct mux *mux_run;
	struct mux *mux_act;
	struct task_stack task_stack;
};

struct nosv_cpu {
	struct chan *ch;
	struct mux *mux;
};

struct nosv_proc {
	struct task_info task_info;
};

int nosv_probe(struct emu *emu);
int nosv_create(struct emu *emu);
int nosv_connect(struct emu *emu);
int nosv_event(struct emu *emu);
int nosv_finish(struct emu *emu);

int nosv_init_pvt(struct emu *emu);
int nosv_finish_pvt(struct emu *emu);
const char *nosv_ss_name(int ss);

#endif /* NOSV_PRIV_H */
