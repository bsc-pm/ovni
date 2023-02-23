/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_STAT_H
#define EMU_STAT_H

#include "player.h"

/* Easier to parse emulation event */
struct emu_stat {
	double in_progress;
	double period;
	double last_time;
	double reported_time;
	double first_time;
	int64_t last_nprocessed;
	int64_t ncalls;
	int64_t maxcalls;
	int average;
};

void emu_stat_init(struct emu_stat *stat);
void emu_stat_update(struct emu_stat *stat, struct player *player);
void emu_stat_report(struct emu_stat *stat, struct player *player, int last);

#endif /* EMU_STAT_H */
