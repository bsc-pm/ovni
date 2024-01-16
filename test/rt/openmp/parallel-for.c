#include "compat.h"

int main(void)
{
	#pragma omp parallel
	{
		#pragma omp for
		for (int i = 0; i < 100; i++) {
			sleep_us(1);
		}

		#pragma omp for schedule(dynamic, 1)
		for (int i = 0; i < 100; i++) {
			sleep_us(i);
		}

		#pragma omp for
		for (int i = 0; i < 100; i++) {
			sleep_us(1);
		}

		#pragma omp for schedule(dynamic, 1)
		for (int i = 0; i < 100; i++) {
			sleep_us(i);
		}
	}

	return 0;
}
