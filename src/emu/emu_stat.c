/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu_stat.h"
#include <string.h>
#include <time.h>
#include "common.h"
#include "player.h"

static double
get_time(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + (double) ts.tv_nsec * 1.0e-9;
}

void
emu_stat_init(struct emu_stat *stat)
{
	memset(stat, 0, sizeof(struct emu_stat));

	double t = get_time();
	stat->last_time = t;
	stat->reported_time = t;
	stat->first_time = t;
	stat->maxcalls = 100;
	stat->period = 0.2;
	stat->average = 1; /* Show average speed */
}

void
emu_stat_report(struct emu_stat *stat, struct player *player, int last)
{
	double progress = player_progress(player);
	int64_t nprocessed = player_nprocessed(player);

	double time_now = get_time();
	double time_elapsed = time_now - stat->first_time;
	double time_total = time_elapsed / progress;
	double time_left = time_total - time_elapsed;

	int64_t delta_nprocessed = nprocessed - stat->last_nprocessed;
	double time_delta = time_now - stat->reported_time;
	double instspeed = 0.0;
	if (time_delta > 0.0)
		instspeed = (double) delta_nprocessed / time_delta;

	/* Compute average speed since the beginning */
	double avgspeed = 0.0;
	if (time_elapsed > 0.0)
		avgspeed = (double) nprocessed / time_elapsed;

	double speed = stat->average ? avgspeed : instspeed;

	if (last) {
		info("%5.1f%% done at avg %.0f kev/s                  \n",
				progress * 100.0, avgspeed * 1e-3);
		info("processed %"PRIi64" input events in %.2f s\n",
				nprocessed, time_elapsed);
	} else {
		int tmin = (int) (time_left / 60.0);
		double tsec = ((time_left / 60.0 - tmin) * 60.0);
		info("%5.1f%% done at %.0f kev/s (%d min %.1f s left)   \r",
				progress * 100.0, speed * 1e-3, tmin, tsec);
	}

	stat->reported_time = time_now;
	stat->last_nprocessed = nprocessed;
}

void
emu_stat_update(struct emu_stat *stat, struct player *player)
{
	stat->ncalls++;

	/* Don't do anything until we arrive at a minimum number of calls */
	if (stat->ncalls < stat->maxcalls)
		return;

	stat->ncalls = 0;

	double t = get_time();

	stat->last_time = t;

	/* Not yet */
	if (stat->last_time - stat->reported_time < stat->period)
		return;

	emu_stat_report(stat, player, 0);
}
