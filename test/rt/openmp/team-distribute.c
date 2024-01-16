#include <omp.h>
#include "compat.h"

int main(void)
{
	#pragma omp teams num_teams(2)
	{
		#pragma omp distribute parallel for
		for (volatile int i = 0; i < 1000; i++)
			sleep_us(100 + i);
	}

	return 0;
}
