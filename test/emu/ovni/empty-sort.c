/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

#include <limits.h>
#include <linux/limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include "compat.h"
#include "ovni.h"

static inline void
init(void)
{
	char hostname[HOST_NAME_MAX];

	if (gethostname(hostname, HOST_NAME_MAX) != 0) {
		perror("gethostname failed");
		exit(EXIT_FAILURE);
	}

	ovni_proc_init(0, hostname, getpid());
	ovni_thread_init(gettid());
	ovni_add_cpu(0, 0);
}

static void
emit(char *mcv, int64_t clock)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, mcv);
	ovni_ev_set_clock(&ev, clock);
	ovni_ev_emit(&ev);
}

int
main(void)
{
	init();

	/* Leave some room to prevent clashes */
	usleep(100); /* 100000 us */

	int64_t t0 = ovni_clock_now();

	emit("OU[", t0);

	int64_t t = t0 - 10000;
	for (int i = 0; i < 100; i++) {
		emit("OB.", t);
		t += 33;
	}

	emit("OU]", ovni_clock_now());

	ovni_flush();
	ovni_proc_fini();

	return 0;
}
