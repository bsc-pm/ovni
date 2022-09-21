#define _GNU_SOURCE
#include <unistd.h>

int main(void)
{
	#pragma oss task if(0)
	{
		usleep(1000);
	}
	return 0;
}
