#include <stdio.h>
#include "compat.h"

int main(void)
{
	#pragma omp parallel sections
	{
		#pragma omp section
		{ sleep_us(1001); printf("1001\n"); }
		#pragma omp section
		{ sleep_us(1002); printf("1002\n"); }
		#pragma omp section
		{ sleep_us(1003); printf("1003\n"); }
		#pragma omp section
		{ sleep_us(1004); printf("1004\n"); }
		#pragma omp section
		sleep_us(1005);
		#pragma omp section
		sleep_us(1006);
		#pragma omp section
		sleep_us(1007);
		#pragma omp section
		sleep_us(1008);
		#pragma omp section
		sleep_us(1009);
		#pragma omp section
		sleep_us(1010);
		#pragma omp section
		sleep_us(1011);
		#pragma omp section
		sleep_us(1012);
		#pragma omp section
		sleep_us(1013);
		#pragma omp section
		sleep_us(1014);
		#pragma omp section
		sleep_us(1015);
		#pragma omp section
		sleep_us(1016);
		#pragma omp section
		sleep_us(1017);
		#pragma omp section
		sleep_us(1018);
		#pragma omp section
		sleep_us(1019);
		#pragma omp section
		sleep_us(1020);
	}

	return 0;
}
