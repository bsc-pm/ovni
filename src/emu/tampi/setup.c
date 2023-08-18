/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "tampi_priv.h"
#include <stddef.h>
#include "chan.h"
#include "common.h"
#include "emu.h"
#include "emu_args.h"
#include "emu_prv.h"
#include "extend.h"
#include "model.h"
#include "model_chan.h"
#include "model_cpu.h"
#include "model_pvt.h"
#include "model_thread.h"
#include "pv/pcf.h"
#include "pv/prv.h"
#include "system.h"
#include "thread.h"
#include "track.h"
#include "value.h"

static const char model_name[] = "tampi";
enum { model_id = 'T' };

struct model_spec model_tampi = {
	.name = model_name,
	.model = model_id,
	.create  = model_tampi_create,
//	.connect = model_tampi_connect,
	.event   = model_tampi_event,
	.probe   = model_tampi_probe,
	.finish  = model_tampi_finish,
};

/* ----------------- channels ------------------ */

static const char *chan_name[CH_MAX] = {
	[CH_SUBSYSTEM] = "subsystem",
};

static const int chan_stack[CH_MAX] = {
	[CH_SUBSYSTEM] = 1,
};

/* ----------------- pvt ------------------ */

static const int pvt_type[CH_MAX] = {
	[CH_SUBSYSTEM] = PRV_TAMPI_SUBSYSTEM,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_SUBSYSTEM]   = "TAMPI subsystem",
};

static const struct pcf_value_label tampi_ss_values[] = {
	{ ST_COMM_ISSUE_NONBLOCKING, "Communication: Issuing a non-blocking operation" },
	{ ST_GLOBAL_ARRAY_CHECK,     "Global array: Checking pending requests" },
	{ ST_LIBRARY_INTERFACE,      "Library code: Interface function" },
	{ ST_LIBRARY_POLLING,        "Library code: Polling function" },
	{ ST_QUEUE_ADD,              "Queue: Adding to a queue" },
	{ ST_QUEUE_TRANSFER,         "Queue: Transfering to global array" },
	{ ST_REQUEST_TEST,           "Request: Testing a request" },
	{ ST_REQUEST_COMPLETED,      "Request: Processing a completed request" },
	{ ST_REQUEST_TESTALL,        "Request: Testing all requests" },
	{ ST_REQUEST_TESTSOME,       "Request: Testing some requests" },
	{ ST_TICKET_CREATE,          "Ticket: Creating a ticket" },
	{ ST_TICKET_WAIT,            "Ticket: Waiting a ticket" },
	{ -1, NULL },
};

static const struct pcf_value_label *pcf_labels[CH_MAX] = {
	[CH_SUBSYSTEM] = tampi_ss_values,
};

static const long prv_flags[CH_MAX] = {
	[CH_SUBSYSTEM] = PRV_SKIPDUP,
};

static const struct model_pvt_spec pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
	.flags = prv_flags,
};

/* ----------------- tracking ------------------ */

static const int th_track[CH_MAX] = {
	[CH_SUBSYSTEM] = TRACK_TH_ACT,
};

static const int cpu_track[CH_MAX] = {
	[CH_SUBSYSTEM] = TRACK_TH_RUN,
};

/* ----------------- chan_spec ------------------ */

static const struct model_chan_spec th_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.pvt = &pvt_spec,
	.track = th_track,
};

static const struct model_chan_spec cpu_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.pvt = &pvt_spec,
	.track = cpu_track,
};

/* ----------------- models ------------------ */

static const struct model_cpu_spec cpu_spec = {
	.size = sizeof(struct tampi_cpu),
	.chan = &cpu_chan,
	.model = &model_tampi,
};

static const struct model_thread_spec th_spec = {
	.size = sizeof(struct tampi_thread),
	.chan = &th_chan,
	.model = &model_tampi,
};

/* ----------------------------------------------------- */

int
model_tampi_probe(struct emu *emu)
{
	if (emu->system.nthreads == 0)
		return 1;

	return 0;
}

int
model_tampi_create(struct emu *emu)
{
	if (model_thread_create(emu, &th_spec) != 0) {
		err("model_thread_init failed");
		return -1;
	}

	if (model_cpu_create(emu, &cpu_spec) != 0) {
		err("model_cpu_init failed");
		return -1;
	}

	return 0;
}

int
model_tampi_connect(struct emu *emu)
{
	if (model_thread_connect(emu, &th_spec) != 0) {
		err("model_thread_connect failed");
		return -1;
	}

	if (model_cpu_connect(emu, &cpu_spec) != 0) {
		err("model_cpu_connect failed");
		return -1;
	}

	return 0;
}

static int
end_lint(struct emu *emu)
{
	/* Only run the check if we finished the complete trace */
	if (!emu->finished)
		return 0;

	struct system *sys = &emu->system;

	/* Ensure we run out of subsystem states */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		struct tampi_thread *th = EXT(t, model_id);
		struct chan *ch = &th->m.ch[CH_SUBSYSTEM];
		int stacked = ch->data.stack.n;
		if (stacked > 0) {
			struct value top;
			if (chan_read(ch, &top) != 0) {
				err("chan_read failed for subsystem");
				return -1;
			}

			err("thread %d ended with %d stacked tampi subsystems",
					t->tid, stacked);
			return -1;
		}
	}

	return 0;
}

int
model_tampi_finish(struct emu *emu)
{
	/* When running in linter mode perform additional checks */
	if (emu->args.linter_mode && end_lint(emu) != 0) {
		err("end_lint failed");
		return -1;
	}

	return 0;
}
