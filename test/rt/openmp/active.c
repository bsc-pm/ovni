#include "compat.h"

/* This test tries to make threads generate as many nosv_pause() calls to try to
 * flood the trace as possible. */

int main(void)
{
	#pragma omp parallel
	#pragma omp single nowait
	{
		#pragma omp task
		{
			sleep_us(10000);
		}
		sleep_us(10000);
	}

	return 0;
}
