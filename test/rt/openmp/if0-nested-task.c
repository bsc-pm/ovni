#include "compat.h"

int main(void)
{
	#pragma omp parallel
	#pragma omp single
	{
		#pragma omp task if(0)
		{
			#pragma omp task
			{
				sleep_us(1000);
			}
			#pragma omp taskwait
		}
	}

	return 0;
}

