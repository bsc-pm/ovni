#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/limits.h>

#include "ovni.h"

#ifndef gettid
# include <sys/syscall.h>
# define gettid() ((pid_t)syscall(SYS_gettid))
#endif

static inline void
init()
{
	char hostname[HOST_NAME_MAX];
	char *appid;

	if(gethostname(hostname, HOST_NAME_MAX) != 0)
	{
		perror("gethostname failed");
		abort();
	}

	ovni_proc_init(0, hostname, getpid());
	ovni_thread_init(gettid());
	ovni_add_cpu(0, 0);
}

int main(int argc, char *argv[])
{
	struct ovni_ev ev = {0};
	int i, n;

	if(argv[1] != NULL)
		n = atoi(argv[1]);
	else
		n = 1000;

	init();

	ovni_ev_set_mcv(&ev, "OB.");

	for(i=0; i<n; i++)
	{
		ovni_clock_update();
		ovni_ev(&ev);
	}

	ovni_flush();
	ovni_proc_fini();

	return 0;
}
