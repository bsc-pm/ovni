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
};

void emu_stat_init(struct emu_stat *stat, double period_seconds);
void emu_stat_update(struct emu_stat *stat, struct player *player);
void emu_stat_report(struct emu_stat *stat, struct player *player);

#endif /* EMU_STAT_H */
