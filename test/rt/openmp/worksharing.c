#include "compat.h"

int main(void)
{
	#pragma omp parallel
	{
		#pragma omp for schedule(dynamic) ordered label("omp for dynamic")
		for (int i = 0; i < 100; i++)
			sleep_us(100);

		#pragma omp single label("single")
			sleep_us(1000);
	}

	return 0;
}
