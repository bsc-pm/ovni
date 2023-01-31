/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MODEL_NANOS6_H
#define MODEL_NANOS6_H

#include "emu_model.h"

extern struct model_spec model_nanos6;

#include "chan.h"
#include "mux.h"
#include "task.h"

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

#endif /* MODEL_NANOS6_H */
