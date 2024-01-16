#include "compat.h"

int main(void)
{
	#pragma omp parallel
	{
		#pragma omp loop
		for (int i = 0; i < 100; i++) {
			sleep_us(1);
		}
	}

	return 0;
}
