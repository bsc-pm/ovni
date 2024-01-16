#include "compat.h"

int main(void)
{
	int a;
	int *p = &a;

	#pragma omp parallel
	#pragma omp single
	{
		#pragma omp task depend(out : p[0])
		{
			sleep_us(1000);
		}
		for (int i = 0; i < 10000; i++)
		{
			#pragma omp task depend(in : p[0])
			sleep_us(1);
		}
	}

	return 0;
}
