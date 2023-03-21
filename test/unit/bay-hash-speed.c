/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 200112L

#include "emu/bay.h"
#include "common.h"
#include "unittest.h"
#include <time.h>

#define N 10000
#define BASE "testchannelultramegahyperverylongname"

struct chan *channels = NULL;
int64_t dummy_value = 1;

static double
get_time(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double) ts.tv_sec + (double) ts.tv_nsec * 1.0e-9;
}

static void
populate(struct bay *bay)
{
	char name[MAX_CHAN_NAME];

	channels = calloc(N, sizeof(struct chan));
	if (channels == NULL)
		die("calloc failed");

	for (long i = 0; i < N; i++) {
		sprintf(name, "%s.%ld", BASE, i);
		chan_init(&channels[i], CHAN_SINGLE, name);
		OK(bay_register(bay, &channels[i]));
	}
}

static void
dummy_work(struct chan *c)
{
	OK(chan_set(c, value_int64(dummy_value++)));
	OK(chan_flush(c));
}

static double
measure_hash(struct bay *bay, double T)
{
	double t0 = get_time();
	double t1 = t0 + T;

	char name[MAX_CHAN_NAME];
	long nlookups = 0;
	srand(1);

	while (get_time() < t1) {
		long i = rand() % N;
		sprintf(name, "%s.%ld", BASE, i);
		struct chan *c = bay_find(bay, name);
		if (c == NULL)
			die("bay_find failed");
		dummy_work(c);
		nlookups++;
	}

	double speed = (double) nlookups / T;

	err("%e lookups/s", speed);
	return speed;
}

static double
measure_direct(double T)
{
	double t0 = get_time();
	double t1 = t0 + T;

	long nlookups = 0;
	srand(1);

	while (get_time() < t1) {
		long i = rand() % N;
		dummy_work(&channels[i]);
		nlookups++;
	}

	double speed = (double) nlookups / T;

	err("%e lookups/s", speed);
	return speed;
}

static void
test_speed(struct bay *bay)
{
	double T = 0.200;
	double hash = measure_hash(bay, T);
	double direct = measure_direct(T);
	double slowdown = hash / direct;
	err("slowdown speed_hash/speed_direct = %f", slowdown);

	if (slowdown < 0.2)
		die("hash speed is too slow");
}

int main(void)
{
	struct bay bay;
	bay_init(&bay);

	populate(&bay);
	test_speed(&bay);

	return 0;
}
