/*
 * Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <limits.h>

#include "ovni.h"

#ifndef gettid
# include <sys/syscall.h>
# define gettid() ((pid_t)syscall(SYS_gettid))
#endif

static inline void
init(void)
{
	char hostname[HOST_NAME_MAX];

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

	if(argc > 1)
		n = atoi(argv[1]);
	else
		n = 1000;

	init();

	ovni_ev_set_mcv(&ev, "OB.");

	for(i=0; i<n; i++)
	{
		ovni_ev_set_clock(&ev, ovni_clock_now());
		ovni_ev_emit(&ev);
	}

	ovni_flush();
	ovni_proc_fini();

	return 0;
}
