/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MPI_PRIV_H
#define MPI_PRIV_H

#include "emu.h"
#include "model_cpu.h"
#include "model_thread.h"

/* Private enums */

enum mpi_chan {
	CH_FUNCTION = 0,
	CH_MAX,
};

enum mpi_function_values {
	ST_MPI_INIT = 1,
	ST_MPI_INIT_THREAD,
	ST_MPI_FINALIZE,
	ST_MPI_WAIT,
	ST_MPI_WAITALL,
	ST_MPI_WAITANY,
	ST_MPI_WAITSOME,
	ST_MPI_TEST,
	ST_MPI_TESTALL,
	ST_MPI_TESTANY,
	ST_MPI_TESTSOME,
	ST_MPI_RECV,
	ST_MPI_SEND,
	ST_MPI_BSEND,
	ST_MPI_RSEND,
	ST_MPI_SSEND,
	ST_MPI_SENDRECV,
	ST_MPI_SENDRECV_REPLACE,
	ST_MPI_IRECV,
	ST_MPI_ISEND,
	ST_MPI_IBSEND,
	ST_MPI_IRSEND,
	ST_MPI_ISSEND,
	ST_MPI_ISENDRECV,
	ST_MPI_ISENDRECV_REPLACE,
	ST_MPI_ALLGATHER,
	ST_MPI_ALLREDUCE,
	ST_MPI_ALLTOALL,
	ST_MPI_BARRIER,
	ST_MPI_BCAST,
	ST_MPI_GATHER,
	ST_MPI_REDUCE,
	ST_MPI_REDUCE_SCATTER,
	ST_MPI_REDUCE_SCATTER_BLOCK,
	ST_MPI_SCATTER,
	ST_MPI_SCAN,
	ST_MPI_EXSCAN,
	ST_MPI_IALLGATHER,
	ST_MPI_IALLREDUCE,
	ST_MPI_IALLTOALL,
	ST_MPI_IBARRIER,
	ST_MPI_IBCAST,
	ST_MPI_IGATHER,
	ST_MPI_IREDUCE,
	ST_MPI_IREDUCE_SCATTER,
	ST_MPI_IREDUCE_SCATTER_BLOCK,
	ST_MPI_ISCATTER,
	ST_MPI_ISCAN,
	ST_MPI_IEXSCAN,
};

struct mpi_thread {
	struct model_thread m;
};

struct mpi_cpu {
	struct model_cpu m;
};

int model_mpi_probe(struct emu *emu);
int model_mpi_create(struct emu *emu);
int model_mpi_connect(struct emu *emu);
int model_mpi_event(struct emu *emu);
int model_mpi_finish(struct emu *emu);

#endif /* MPI_PRIV_H */
