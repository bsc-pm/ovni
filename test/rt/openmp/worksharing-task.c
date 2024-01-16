#include <omp.h>
#include "compat.h"

static void foo(void)
{
	#pragma omp for
	for (int i = 0; i < 100; ++i)
	{
		#pragma omp task
		{
			sleep_us(1);
		}
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
