#include <omp.h>
#include <stdio.h>
#include "compat.h"

/* Test several work-distribution and task constructs, so we can generate a
 * trace that includes most of the states. */

int main(void)
{
	#pragma omp parallel
	{
		#pragma omp for label("static-for-1")
		for (int i = 0; i < 100; i++) {
			sleep_us(1);
		}

		#pragma omp sections
		{
			#pragma omp section
			{ sleep_us(101); printf("101\n"); }
			#pragma omp section
			{ sleep_us(102); printf("102\n"); }
			#pragma omp section
			{ sleep_us(103); printf("103\n"); }
			#pragma omp section
			{ sleep_us(104); printf("104\n"); }
		}

		#pragma omp for label("static-for-2")
		for (int i = 0; i < 100; i++) {
			sleep_us(1);
		}

		#pragma omp single
		for (int i = 0; i < 100; i++)
		{
			#pragma omp task label("mini-task")
			sleep_us(10);
		}
	}

	#pragma omp parallel
	{
		#pragma omp critical
		sleep_us(20);

		#pragma omp barrier

		#pragma omp for label("static-for-3")
		for (int i = 0; i < 100; i++) {
			sleep_us(1);
		}
		#pragma omp for schedule(dynamic, 1) label("dynamic-for")
		for (int i = 0; i < 100; i++) {
			sleep_us(i);
		}
	}

	// FIXME: Crashes OpenMP-V runtime
	//#pragma omp distribute parallel for
	//for (int i = 0; i < 1000; i++) {
	//	sleep_us(1);
	//}

	return 0;
}
