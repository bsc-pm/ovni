#include "compat.h"

int main(void)
{
	#pragma omp parallel
	#pragma omp single
	{
/* Broken clang: https://github.com/llvm/llvm-project/issues/100536 */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
		#pragma omp taskloop
		for (int i = 0; i < 10000; i++)
		{
			#pragma omp task
			sleep_us(1);
		}
#pragma clang diagnostic pop
	}

	return 0;
}
