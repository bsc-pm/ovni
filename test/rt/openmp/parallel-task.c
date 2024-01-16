#include "compat.h"

static void foo(void)
{
	#pragma omp for schedule(dynamic, 1)
	for (int i = 0; i < 100; i++)
		sleep_us(i);

	#pragma omp single
	for (int i = 0; i < 100; i++)
	{
		#pragma omp task
		sleep_us(10);
	}
}

int main(void)
{
	#pragma omp parallel
	{
		foo();
		foo();
	}

	return 0;
}
