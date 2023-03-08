#ifndef BREAKDOWN_H
#define BREAKDOWN_H

/* 
 * The breakdown model is implemented on top of the CPU subsystem, task_type and
 * idle channels. The first mux0 selects the task type when the subsystem
 * matches "Task body" otherwise forwards the subsystem as-is to tr. The second
 * mux1 selects tr only when the CPU is not Idle, otherwise sets the output tri
 * as Idle.
 *
 *            +--------+
 *            |        |
 *            |        v
 *            |     +------+
 * subsystem -+-->--|      |
 *                  | mux0 |              +------+
 * task_type ---->--|      |-->-- tr -->--|      |
 *                  +------+              | mux1 |-->-- tri
 * idle --------->-------------------+->--|      |
 *                                   |    +------+
 *                                   |        ^
 *                                   |        |
 *                                   +--------+
 *
 * Then the sort module takes the output tri of each CPU and sorts the values
 * which are propagated to the PRV directly.
 *
 *                 +------+       +-----+
 * cpu0.tri --->---|      |--->---|     |
 * ...             | sort |  ...  | PRV |
 * cpuN.tri --->---|      |--->---|     |
 *                 +------+       +-----+
 */

#include "mux.h"
#include "chan.h"
#include "sort.h"
#include "pv/pvt.h"

struct breakdown_cpu {
	struct mux  mux0;
	struct chan tr;
	struct mux  mux1;
	struct chan tri;
};

struct breakdown_emu {
	int64_t nphycpus;
	struct sort sort;
	struct pvt *pvt;
};

#endif /* BREAKDOWN_H */
