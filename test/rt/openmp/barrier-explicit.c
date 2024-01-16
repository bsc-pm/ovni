#include <stdio.h>
#include <math.h>
#include "compat.h"

#define N 100

static void
dummy_work(double *x, int i)
{
	sleep_us(i);
	x[i] += sqrt((double) i);
}

int main(void)
{
	double x[N] = { 0 };
	#pragma omp parallel
	{
		#pragma omp single
		{
			for (int i = 0; i < N; i++) {
				#pragma omp task shared(x)
				dummy_work(x, i);
			}
		}

		sleep_us(200);
		#pragma omp barrier
		sleep_us(1000);
		#pragma omp barrier

		#pragma omp single
		{
			for (int i = 0; i < N; i++) {
				#pragma omp task shared(x)
				dummy_work(x, i);
			}
		}
	}

	double sum = 0.0;
	for (int i = 0; i < N; i++)
		sum += x[i];

	printf("sum = %e\n", sum);
	return 0;
}
