#include <stdio.h>
#include "compat.h"

int main(void)
{
	#pragma omp parallel
	{
		for (int i = 0; i < 10; i++) {
			#pragma omp task
			{
				printf("%d\n", i);
				sleep_us(100);
			}
		}
		#pragma omp barrier
	}

	return 0;
}
