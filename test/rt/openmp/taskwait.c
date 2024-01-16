#include "compat.h"
#include <stdio.h>

int main(void)
{
	#pragma omp parallel
	#pragma omp single
	{
		#pragma omp task label("A")
		{
			sleep_us(5000);
			printf("A\n");
		}

		#pragma omp task label("B")
		{
			#pragma omp task label("B1")
			{
				sleep_us(2000);
				printf("B1\n");
			}

			/* Shouldn't wait for task A */
			#pragma omp taskwait

			#pragma omp task
			{
				sleep_us(1000);
				printf("B2\n");
			}
		}

		#pragma omp task label("C")
		{
			printf("C\n");
		}
	}

	/* Expected output C B1 B2 A */

	return 0;
}
