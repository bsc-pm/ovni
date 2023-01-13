#define _POSIX_C_SOURCE 200112L

#include "emu/bay.h"
#include "common.h"
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
		die("calloc failed\n");

	for (long i = 0; i < N; i++) {
		sprintf(name, "%s.%ld", BASE, i);
		chan_init(&channels[i], CHAN_SINGLE, name);
		if (bay_register(bay, &channels[i]) != 0)
			die("bay_register failed\n");
	}
}

static void
dummy_work(struct chan *c)
{
	if (chan_set(c, value_int64(dummy_value++)) != 0)
		die("chan_set failed\n");
	if (chan_flush(c) != 0)
		die("chan_flush failed\n");
}

static double
measure_hash(struct bay *bay)
{
	double t0 = get_time();
	double T = 5.0;
	double t1 = t0 + T;

	char name[MAX_CHAN_NAME];
	long nlookups = 0;
	srand(1);

	while (get_time() < t1) {
		long i = rand() % N;
		sprintf(name, "%s.%ld", BASE, i);
		struct chan *c = bay_find(bay, name);
		if (c == NULL)
			die("bay_find failed\n");
		dummy_work(c);
		nlookups++;
	}

	double speed = (double) nlookups / T;

	err("bay_find: %e lookups/s\n", speed);
	return speed;
}

static double
measure_direct(void)
{
	double t0 = get_time();
	double T = 5.0;
	double t1 = t0 + T;

	long nlookups = 0;
	srand(1);

	while (get_time() < t1) {
		long i = rand() % N;
		dummy_work(&channels[i]);
		nlookups++;
	}

	double speed = (double) nlookups / T;

	err("direct: %e lookups/s\n", speed);
	return speed;
}

static void
test_speed(struct bay *bay)
{
	double hash = measure_hash(bay);
	double direct = measure_direct();
	double slowdown = hash / direct;
	err("slowdown speed_hash/speed_direct = %f\n", slowdown);

	if (slowdown < 0.2)
		die("hash speed is too slow\n");
}

int main(void)
{
	struct bay bay;
	bay_init(&bay);

	populate(&bay);
	test_speed(&bay);

	return 0;
}
