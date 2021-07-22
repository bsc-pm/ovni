#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

#include "ovni.h"

#define N 1000

int main()
{
	int i;

	ovni_proc_init(0, 0);
	ovni_thread_init(1);

	for(i=0; i<N; i++)
	{
		ovni_clock_update();
		ovni_thread_ev('B', '.', 0, 0);
	}

	ovni_thread_flush();

	return 0;
}
