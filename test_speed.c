#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <linux/limits.h>

#include "ovni.h"

#define N 100000

int main()
{
	struct ovni_ev ev = {0};
	int i;

	ovni_proc_init(0, "test", 0);
	ovni_thread_init(1);

	ovni_ev_set_mcv(&ev, "OB.");

	for(i=0; i<N; i++)
	{
		ovni_clock_update();
		ovni_ev(&ev);
	}

	ovni_flush();

	return 0;
}
