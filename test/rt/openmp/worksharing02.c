#include "compat.h"

int main(void)
{
	#pragma omp target
	#pragma omp teams num_teams(1)
	#pragma omp distribute dist_schedule(static, 30)
	for (int i = 0; i < 100; i++)
		sleep_us(10);

	return 0;
}
