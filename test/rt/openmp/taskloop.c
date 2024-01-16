#include "compat.h"

int main(void)
{
	#pragma omp parallel
	#pragma omp single
	{
		#pragma omp taskloop
		for (int i = 0; i < 10000; i++)
		{
			#pragma omp task
			sleep_us(1);
		}
	}

	return 0;
}
