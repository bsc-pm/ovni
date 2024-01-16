#include <omp.h>
#include "compat.h"

int main(void)
{
	#pragma omp parallel
	#pragma omp single
	{
		#pragma omp task if(0)
		{
			sleep_us(1000);
		}
	}

	return 0;
}

