#include "compat.h"
#include <omp.h>

/* This test tries to make threads generate as many nosv_pause() calls to try to
 * flood the trace as possible. This problem should be solved by the new passive
 * mechanism. */

int main(void)
{
	#pragma omp parallel
	{
		if (omp_get_thread_num() == 1)
		#pragma omp task
		{
			sleep_us(10000);
		}
	}

	return 0;
}
