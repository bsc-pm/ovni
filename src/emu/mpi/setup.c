/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "mpi_priv.h"
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

static const char model_name[] = "mpi";
enum { model_id = 'M' };

struct model_spec model_mpi = {
	.name = model_name,
	.model = model_id,
	.create  = model_mpi_create,
//	.connect = model_mpi_connect,
	.event   = model_mpi_event,
	.probe   = model_mpi_probe,
	.finish  = model_mpi_finish,
};

/* ----------------- channels ------------------ */

static const char *chan_name[CH_MAX] = {
	[CH_FUNCTION] = "function",
};

static const int chan_stack[CH_MAX] = {
	[CH_FUNCTION] = 1,
};

/* ----------------- pvt ------------------ */

static const int pvt_type[CH_MAX] = {
	[CH_FUNCTION] = PRV_MPI_FUNCTION,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_FUNCTION] = "MPI function",
};

static const struct pcf_value_label mpi_function_values[] = {
	{ ST_MPI_INIT,                  "MPI_Init" },
	{ ST_MPI_INIT_THREAD,           "MPI_Init_thread" },
	{ ST_MPI_FINALIZE,              "MPI_Finalize" },
	{ ST_MPI_WAIT,                  "MPI_Wait" },
	{ ST_MPI_WAITALL,               "MPI_Waitall" },
	{ ST_MPI_WAITANY,               "MPI_Waitany" },
	{ ST_MPI_WAITSOME,              "MPI_Waitsome" },
	{ ST_MPI_TEST,                  "MPI_Test" },
	{ ST_MPI_TESTALL,               "MPI_Testall" },
	{ ST_MPI_TESTANY,               "MPI_Testany" },
	{ ST_MPI_TESTSOME,              "MPI_Testsome" },
	{ ST_MPI_RECV,                  "MPI_Recv" },
	{ ST_MPI_SEND,                  "MPI_Send" },
	{ ST_MPI_BSEND,                 "MPI_Bsend" },
	{ ST_MPI_RSEND,                 "MPI_Rsend" },
	{ ST_MPI_SSEND,                 "MPI_Ssend" },
	{ ST_MPI_SENDRECV,              "MPI_Sendrecv" },
	{ ST_MPI_SENDRECV_REPLACE,      "MPI_Sendrecv_replace" },
	{ ST_MPI_IRECV,                 "MPI_Irecv" },
	{ ST_MPI_ISEND,                 "MPI_Isend" },
	{ ST_MPI_IBSEND,                "MPI_Ibsend" },
	{ ST_MPI_IRSEND,                "MPI_Irsend" },
	{ ST_MPI_ISSEND,                "MPI_Issend" },
	{ ST_MPI_ISENDRECV,             "MPI_Isendrecv" },
	{ ST_MPI_ISENDRECV_REPLACE,     "MPI_Isendrecv_replace" },
	{ ST_MPI_ALLGATHER,             "MPI_Allgather" },
	{ ST_MPI_ALLREDUCE,             "MPI_Allreduce" },
	{ ST_MPI_ALLTOALL,              "MPI_Alltoall" },
	{ ST_MPI_BARRIER,               "MPI_Barrier" },
	{ ST_MPI_BCAST,                 "MPI_Bcast" },
	{ ST_MPI_GATHER,                "MPI_Gather" },
	{ ST_MPI_REDUCE,                "MPI_Reduce" },
	{ ST_MPI_REDUCE_SCATTER,        "MPI_Reduce_scatter" },
	{ ST_MPI_REDUCE_SCATTER_BLOCK,  "MPI_Reduce_scatter_block" },
	{ ST_MPI_SCATTER,               "MPI_Scatter" },
	{ ST_MPI_SCAN,                  "MPI_Scan" },
	{ ST_MPI_EXSCAN,                "MPI_Exscan" },
	{ ST_MPI_IALLGATHER,            "MPI_Iallgather" },
	{ ST_MPI_IALLREDUCE,            "MPI_Iallreduce" },
	{ ST_MPI_IALLTOALL,             "MPI_Ialltoall" },
	{ ST_MPI_IBARRIER,              "MPI_Ibarrier" },
	{ ST_MPI_IBCAST,                "MPI_Ibcast" },
	{ ST_MPI_IGATHER,               "MPI_Igather" },
	{ ST_MPI_IREDUCE,               "MPI_Ireduce" },
	{ ST_MPI_IREDUCE_SCATTER,       "MPI_Ireduce_scatter" },
	{ ST_MPI_IREDUCE_SCATTER_BLOCK, "MPI_Ireduce_scatter_block" },
	{ ST_MPI_ISCATTER,              "MPI_Iscatter" },
	{ ST_MPI_ISCAN,                 "MPI_Iscan" },
	{ ST_MPI_IEXSCAN,               "MPI_Iexscan" },
	{ -1, NULL },
};

static const struct pcf_value_label *pcf_labels[CH_MAX] = {
	[CH_FUNCTION] = mpi_function_values,
};

static const long prv_flags[CH_MAX] = {
	[CH_FUNCTION] = PRV_SKIPDUP,
};

static const struct model_pvt_spec pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
	.flags = prv_flags,
};

/* ----------------- tracking ------------------ */

static const int th_track[CH_MAX] = {
	[CH_FUNCTION] = TRACK_TH_RUN,
};

static const int cpu_track[CH_MAX] = {
	[CH_FUNCTION] = TRACK_TH_RUN,
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
	.size = sizeof(struct mpi_cpu),
	.chan = &cpu_chan,
	.model = &model_mpi,
};

static const struct model_thread_spec th_spec = {
	.size = sizeof(struct mpi_thread),
	.chan = &th_chan,
	.model = &model_mpi,
};

/* ----------------------------------------------------- */

int
model_mpi_probe(struct emu *emu)
{
	if (emu->system.nthreads == 0)
		return 1;

	return 0;
}

int
model_mpi_create(struct emu *emu)
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
model_mpi_connect(struct emu *emu)
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

	/* Ensure we run out of function states */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		struct mpi_thread *th = EXT(t, model_id);
		struct chan *ch = &th->m.ch[CH_FUNCTION];
		int stacked = ch->data.stack.n;
		if (stacked > 0) {
			struct value top;
			if (chan_read(ch, &top) != 0) {
				err("chan_read failed for function");
				return -1;
			}

			err("thread %d ended with %d stacked mpi functions",
					t->tid, stacked);
			return -1;
		}
	}

	return 0;
}

int
model_mpi_finish(struct emu *emu)
{
	/* When running in linter mode perform additional checks */
	if (emu->args.linter_mode && end_lint(emu) != 0) {
		err("end_lint failed");
		return -1;
	}

	return 0;
}
