/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "compat.h"
#include "ovni.h"
#include "instr.h"

static inline void
init(void)
{
	char hostname[OVNI_MAX_HOSTNAME];

	if (gethostname(hostname, OVNI_MAX_HOSTNAME) != 0) {
		perror("gethostname failed");
		exit(EXIT_FAILURE);
	}

	ovni_proc_init(0, hostname, getpid());
	ovni_thread_init(get_tid());
	ovni_add_cpu(0, 0);
}

static void
emit(char *mcv, int64_t clock)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, mcv);
	ovni_ev_set_clock(&ev, (uint64_t) clock);
	ovni_ev_emit(&ev);
}

int
main(void)
{
	init();

	int64_t t0 = (int64_t) ovni_clock_now();

	/* Leave some room to prevent clashes */
	sleep_us(100000); /* 100000000 ns */

	int64_t t1 = (int64_t) ovni_clock_now();

	emit("OU[", t1);

	/* Fill the ring buffer */
	long n = 1000000 + 10;

	err("using n=%ld events", n);

	/* Go back 100 ns for each event (with some space) */
	int64_t delta = 10000;
	int64_t t = t0 + delta;

	for (long i = 0; i < n; i++) {
		if (t <= t0 || t >= t1)
			die("bad time");
		emit("OB.", t);
		t += 33;
	}

	emit("OU]", (int64_t) ovni_clock_now());

	ovni_flush();
	ovni_proc_fini();

	return 0;
}
