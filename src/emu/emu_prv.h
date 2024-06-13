/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_PRV_H
#define EMU_PRV_H

/* Global table of static PRV types */
enum emu_prv_types {
	PRV_CPU_PID          = 1,
	PRV_CPU_TID          = 2,
	PRV_CPU_NRUN         = 3,
	PRV_THREAD_TID       = PRV_CPU_TID,
	PRV_THREAD_STATE     = 4,
	PRV_THREAD_CPU       = 6,
	PRV_OVNI_FLUSH       = 7,
	PRV_NOSV_TASKID      = 10,
	PRV_NOSV_TYPE        = 11,
	PRV_NOSV_APPID       = 12,
	PRV_NOSV_SUBSYSTEM   = 13,
	PRV_NOSV_RANK        = 14,
	PRV_NOSV_BODYID      = 15,
	PRV_NOSV_IDLE        = 16,
	PRV_NOSV_BREAKDOWN   = 17,
	PRV_TAMPI_SUBSYSTEM  = 20,
	PRV_MPI_FUNCTION     = 25,
	PRV_NODES_SUBSYSTEM  = 30,
	PRV_NANOS6_TASKID    = 35,
	PRV_NANOS6_TYPE      = 36,
	PRV_NANOS6_SUBSYSTEM = 37,
	PRV_NANOS6_RANK      = 38,
	PRV_NANOS6_THREAD    = 39,
	PRV_NANOS6_IDLE      = 40,
	PRV_NANOS6_BREAKDOWN = 41,
	PRV_KERNEL_CS        = 45,
	PRV_OPENMP_SUBSYSTEM = 50,
	PRV_OVNI_MARK        = 100,
	/* User marks [100, 200) */
	PRV_RESERVED         = 200,
};

#endif /* EMU_PRV_H */
