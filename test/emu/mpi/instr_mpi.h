/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef INSTR_MPI_H
#define INSTR_MPI_H

#include "instr.h"

static inline void
instr_mpi_init(void)
{
	instr_require("mpi");
}

INSTR_0ARG(instr_mpi_init_thread_enter, "MUt")
INSTR_0ARG(instr_mpi_init_thread_exit, "MUT")
INSTR_0ARG(instr_mpi_finalize_enter, "MUf")
INSTR_0ARG(instr_mpi_finalize_exit, "MUF")

INSTR_0ARG(instr_mpi_wait_enter, "MW[")
INSTR_0ARG(instr_mpi_wait_exit, "MW]")
INSTR_0ARG(instr_mpi_testsome_enter, "MTs")
INSTR_0ARG(instr_mpi_testsome_exit, "MTS")

INSTR_0ARG(instr_mpi_issend_enter, "Mss")
INSTR_0ARG(instr_mpi_issend_exit, "MsS")
INSTR_0ARG(instr_mpi_irecv_enter, "Mr[")
INSTR_0ARG(instr_mpi_irecv_exit, "Mr]")

INSTR_0ARG(instr_mpi_barrier_enter, "MCb")
INSTR_0ARG(instr_mpi_barrier_exit, "MCB")
INSTR_0ARG(instr_mpi_allreduce_enter, "MAr")
INSTR_0ARG(instr_mpi_allreduce_exit, "MAR")
INSTR_0ARG(instr_mpi_ibcast_enter, "Mdb")
INSTR_0ARG(instr_mpi_ibcast_exit, "MdB")

#endif /* INSTR_MPI_H */
