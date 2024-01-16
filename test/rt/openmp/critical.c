#include "compat.h"

int main(void)
{
	#pragma omp parallel
	{
		sleep_us(1000);

		#pragma omp critical
		sleep_us(200);

		sleep_us(1000);
	}

	return 0;
}
