#include <stdio.h>
#include "compat.h"

int main(void)
{
	#pragma omp parallel
	{
		//#pragma omp single nowait
		for (int i = 0; i < 100; i++) {
			#pragma omp task label("minitask")
			sleep_us(10);
		}

		/* Wait a bit for task allocation */
		sleep_us(1000);

		/* Occupy 4 CPUs with sections */
		#pragma omp sections nowait
		{
			#pragma omp section
			{ sleep_us(1001); printf("1001\n"); }
			#pragma omp section
			{ sleep_us(1002); printf("1002\n"); }
			#pragma omp section
			{ sleep_us(1003); printf("1003\n"); }
			#pragma omp section
			{ sleep_us(1004); printf("1004\n"); }
		}

		#pragma omp taskwait
	}

	return 0;
}
