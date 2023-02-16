/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 200112L

#include "emu_stat.h"

#include <time.h>
#include <string.h>

static double
get_time(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + (double) ts.tv_nsec * 1.0e-9;
}

void
emu_stat_init(struct emu_stat *stat, double period_seconds)
{
	memset(stat, 0, sizeof(struct emu_stat));

	stat->period = period_seconds;
}

void
emu_stat_report(struct emu_stat *stat, struct player *player)
{
	double t = get_time();

	stat->in_progress = player_progress(player);

	double iprog = stat->in_progress * 100.0;

	err("%.1f%% done", iprog);

	stat->last_time = t;
}

void
emu_stat_update(struct emu_stat *stat, struct player *player)
{
	double t = get_time();

	/* Not yet */
	if (t - stat->last_time < stat->period)
		return;

	emu_stat_report(stat, player);
}
