#include "compat.h"

int main(void)
{
	for (int i = 0; i < 10; i++) {
		#pragma omp parallel
		{
			sleep_us(1000);
		}
	}

	return 0;
}
