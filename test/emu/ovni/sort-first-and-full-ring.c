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
#include "common.h"
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

	/* Fill the ring buffer */
	long n = 100000 + 10;

	err("using n=%ld events\n", n);

	/* Go back 100 ns for each event (with some space) */
	int64_t t = t0 - (n + 10) * 100;

	for (long i = 0; i < n; i++) {
		emit("OB.", t);
		t += 33;
	}

	emit("OU]", ovni_clock_now());
	t += 33;

	if (t >= t0)
		die("bad timming\n");

	ovni_flush();
	ovni_proc_fini();

	return 0;
}
