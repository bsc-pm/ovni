#include "compat.h"

int main(void)
{
	#pragma omp parallel
	#pragma omp for schedule(static, 30)
	for (int i = 0; i < 100; i++)
		sleep_us(10);

	return 0;
}
