/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "mpi_priv.h"
#include "chan.h"
#include "common.h"
#include "emu.h"
#include "emu_ev.h"
#include "extend.h"
#include "model_thread.h"
#include "thread.h"
#include "value.h"

enum { PUSH = 1, POP = 2, IGN = 3 };

static const int fn_table[256][256][3] = {
	['U'] = {
		['i'] = { CH_FUNCTION, PUSH, ST_MPI_INIT },
		['I'] = { CH_FUNCTION, POP,  ST_MPI_INIT },
		['t'] = { CH_FUNCTION, PUSH, ST_MPI_INIT_THREAD },
		['T'] = { CH_FUNCTION, POP,  ST_MPI_INIT_THREAD },
		['f'] = { CH_FUNCTION, PUSH, ST_MPI_FINALIZE },
		['F'] = { CH_FUNCTION, POP,  ST_MPI_FINALIZE },
	},
	['W'] = {
		['['] = { CH_FUNCTION, PUSH, ST_MPI_WAIT },
		[']'] = { CH_FUNCTION, POP,  ST_MPI_WAIT },
		['a'] = { CH_FUNCTION, PUSH, ST_MPI_WAITALL },
		['A'] = { CH_FUNCTION, POP,  ST_MPI_WAITALL },
		['y'] = { CH_FUNCTION, PUSH, ST_MPI_WAITANY },
		['Y'] = { CH_FUNCTION, POP,  ST_MPI_WAITANY },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_WAITSOME },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_WAITSOME },
	},
	['T'] = {
		['['] = { CH_FUNCTION, PUSH, ST_MPI_TEST },
		[']'] = { CH_FUNCTION, POP,  ST_MPI_TEST },
		['a'] = { CH_FUNCTION, PUSH, ST_MPI_TESTALL },
		['A'] = { CH_FUNCTION, POP,  ST_MPI_TESTALL },
		['y'] = { CH_FUNCTION, PUSH, ST_MPI_TESTANY },
		['Y'] = { CH_FUNCTION, POP,  ST_MPI_TESTANY },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_TESTSOME },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_TESTSOME },
	},
	['R'] = {
		['['] = { CH_FUNCTION, PUSH, ST_MPI_RECV },
		[']'] = { CH_FUNCTION, POP,  ST_MPI_RECV },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_SENDRECV },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_SENDRECV },
		['o'] = { CH_FUNCTION, PUSH, ST_MPI_SENDRECV_REPLACE },
		['O'] = { CH_FUNCTION, POP,  ST_MPI_SENDRECV_REPLACE },
	},
	['r'] = {
		['['] = { CH_FUNCTION, PUSH, ST_MPI_IRECV },
		[']'] = { CH_FUNCTION, POP,  ST_MPI_IRECV },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_ISENDRECV },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_ISENDRECV },
		['o'] = { CH_FUNCTION, PUSH, ST_MPI_ISENDRECV_REPLACE },
		['O'] = { CH_FUNCTION, POP,  ST_MPI_ISENDRECV_REPLACE },
	},
	['S'] = {
		['['] = { CH_FUNCTION, PUSH, ST_MPI_SEND },
		[']'] = { CH_FUNCTION, POP,  ST_MPI_SEND },
		['b'] = { CH_FUNCTION, PUSH, ST_MPI_BSEND },
		['B'] = { CH_FUNCTION, POP,  ST_MPI_BSEND },
		['r'] = { CH_FUNCTION, PUSH, ST_MPI_RSEND },
		['R'] = { CH_FUNCTION, POP,  ST_MPI_RSEND },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_SSEND },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_SSEND },
	},
	['s'] = {
		['['] = { CH_FUNCTION, PUSH, ST_MPI_ISEND },
		[']'] = { CH_FUNCTION, POP,  ST_MPI_ISEND },
		['b'] = { CH_FUNCTION, PUSH, ST_MPI_IBSEND },
		['B'] = { CH_FUNCTION, POP,  ST_MPI_IBSEND },
		['r'] = { CH_FUNCTION, PUSH, ST_MPI_IRSEND },
		['R'] = { CH_FUNCTION, POP,  ST_MPI_IRSEND },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_ISSEND },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_ISSEND },
	},
	['A'] = {
		['g'] = { CH_FUNCTION, PUSH, ST_MPI_ALLGATHER },
		['G'] = { CH_FUNCTION, POP,  ST_MPI_ALLGATHER },
		['r'] = { CH_FUNCTION, PUSH, ST_MPI_ALLREDUCE },
		['R'] = { CH_FUNCTION, POP,  ST_MPI_ALLREDUCE },
		['a'] = { CH_FUNCTION, PUSH, ST_MPI_ALLTOALL },
		['A'] = { CH_FUNCTION, POP,  ST_MPI_ALLTOALL },
	},
	['a'] = {
		['g'] = { CH_FUNCTION, PUSH, ST_MPI_IALLGATHER },
		['G'] = { CH_FUNCTION, POP,  ST_MPI_IALLGATHER },
		['r'] = { CH_FUNCTION, PUSH, ST_MPI_IALLREDUCE },
		['R'] = { CH_FUNCTION, POP,  ST_MPI_IALLREDUCE },
		['a'] = { CH_FUNCTION, PUSH, ST_MPI_IALLTOALL },
		['A'] = { CH_FUNCTION, POP,  ST_MPI_IALLTOALL },
	},
	['C'] = {
		['b'] = { CH_FUNCTION, PUSH, ST_MPI_BARRIER },
		['B'] = { CH_FUNCTION, POP,  ST_MPI_BARRIER },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_SCAN },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_SCAN },
		['e'] = { CH_FUNCTION, PUSH, ST_MPI_EXSCAN },
		['E'] = { CH_FUNCTION, POP,  ST_MPI_EXSCAN },
	},
	['c'] = {
		['b'] = { CH_FUNCTION, PUSH, ST_MPI_IBARRIER },
		['B'] = { CH_FUNCTION, POP,  ST_MPI_IBARRIER },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_ISCAN },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_ISCAN },
		['e'] = { CH_FUNCTION, PUSH, ST_MPI_IEXSCAN },
		['E'] = { CH_FUNCTION, POP,  ST_MPI_IEXSCAN },
	},
	['D'] = {
		['b'] = { CH_FUNCTION, PUSH, ST_MPI_BCAST },
		['B'] = { CH_FUNCTION, POP,  ST_MPI_BCAST },
		['g'] = { CH_FUNCTION, PUSH, ST_MPI_GATHER },
		['G'] = { CH_FUNCTION, POP,  ST_MPI_GATHER },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_SCATTER },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_SCATTER },
	},
	['d'] = {
		['b'] = { CH_FUNCTION, PUSH, ST_MPI_IBCAST },
		['B'] = { CH_FUNCTION, POP,  ST_MPI_IBCAST },
		['g'] = { CH_FUNCTION, PUSH, ST_MPI_IGATHER },
		['G'] = { CH_FUNCTION, POP,  ST_MPI_IGATHER },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_ISCATTER },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_ISCATTER },
	},
	['E'] = {
		['['] = { CH_FUNCTION, PUSH, ST_MPI_REDUCE },
		[']'] = { CH_FUNCTION, POP,  ST_MPI_REDUCE },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_REDUCE_SCATTER },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_REDUCE_SCATTER },
		['b'] = { CH_FUNCTION, PUSH, ST_MPI_REDUCE_SCATTER_BLOCK },
		['B'] = { CH_FUNCTION, POP,  ST_MPI_REDUCE_SCATTER_BLOCK },
	},
	['e'] = {
		['['] = { CH_FUNCTION, PUSH, ST_MPI_IREDUCE },
		[']'] = { CH_FUNCTION, POP,  ST_MPI_IREDUCE },
		['s'] = { CH_FUNCTION, PUSH, ST_MPI_IREDUCE_SCATTER },
		['S'] = { CH_FUNCTION, POP,  ST_MPI_IREDUCE_SCATTER },
		['b'] = { CH_FUNCTION, PUSH, ST_MPI_IREDUCE_SCATTER_BLOCK },
		['B'] = { CH_FUNCTION, POP,  ST_MPI_IREDUCE_SCATTER_BLOCK },
	},
};

static int
process_ev(struct emu *emu)
{
	if (!emu->thread->is_running) {
		err("current thread %d not running", emu->thread->tid);
		return -1;
	}

	const int *entry = fn_table[emu->ev->c][emu->ev->v];
	int chind = entry[0];
	int action = entry[1];
	int st = entry[2];

	struct mpi_thread *th = EXT(emu->thread, 'M');
	struct chan *ch = &th->m.ch[chind];

	if (action == PUSH) {
		return chan_push(ch, value_int64(st));
	} else if (action == POP) {
		return chan_pop(ch, value_int64(st));
	} else if (action == IGN) {
		return 0; /* do nothing */
	}

	err("unknown mpi function event");
	return -1;
}

int
model_mpi_event(struct emu *emu)
{
	static int enabled = 0;

	if (!enabled) {
		if (model_mpi_connect(emu) != 0) {
			err("mpi_connect failed");
			return -1;
		}
		enabled = 1;
	}

	dbg("in mpi_event");
	if (emu->ev->m != 'M') {
		err("unexpected event model %c", emu->ev->m);
		return -1;
	}

	dbg("got mpi event %s", emu->ev->mcv);
	if (process_ev(emu) != 0) {
		err("error processing mpi event");
		return -1;
	}

	return 0;
}
