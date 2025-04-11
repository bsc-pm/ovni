/* Copyright (c) 2024-2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef BREAKDOWN_H
#define BREAKDOWN_H

/* 
 * The breakdown model is implemented on top of the CPU label and subsystem
 * channels. The mux selects the label when the .
 *
 *                +--------+
 *                |        |
 *                |        v
 *                |     +------+
 *    label ------o-->--|      |
 *                      | mux0 |-->- out
 *    subsystem ----->--|      |
 *                      +------+
 *
 *    mux0 output = label if sel is not null, subsystem otherwise.
 *
 * Then the sort module takes the output of each CPU and sorts the values which
 * are propagated to the PRV directly.
 *
 *                    +------+       +-----+
 *    cpu0.out --->---|      |--->---|     |
 *    ...             | sort |  ...  | PRV |
 *    cpuN.out --->---|      |--->---|     |
 *                    +------+       +-----+
 */

#include <stdint.h>
#include "chan.h"
#include "mux.h"
#include "sort.h"

struct breakdown_cpu {
	struct mux  mux0;
	struct chan out;
};

struct breakdown_emu {
	int64_t nphycpus;
	struct sort sort;
	struct pvt *pvt;
};

#endif /* BREAKDOWN_H */
