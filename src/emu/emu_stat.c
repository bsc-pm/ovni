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
	double t = get_time();
	stat->last_time = t;
	stat->first_time = t;
}

void
emu_stat_report(struct emu_stat *stat, struct player *player)
{
	double progress = player_progress(player);
	int64_t nprocessed = player_nprocessed(player);

	double time_now = get_time();
	double time_elapsed = time_now - stat->first_time;
	double time_total = time_elapsed / progress;
	double time_left = time_total - time_elapsed;

	/* Compute average speed since the beginning */
	double speed = 0.0;
	if (time_elapsed > 0.0)
		speed = (double) nprocessed / time_elapsed;

	verr(NULL, "%5.1f%% done at %.0f kev/s (%.1f s left)",
			progress * 100.0,
			speed * 1e-3,
			time_left);

	stat->last_time = time_now;
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
