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
#include "../instr.h"

static void
emit(char *mcv, uint64_t clock)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, mcv);
	ovni_ev_set_clock(&ev, clock);
	ovni_ev_emit(&ev);
}

int
main(void)
{
	instr_start(0, 1);

	uint64_t t = ovni_clock_now();

	/* Emit bursts with controlled clocks */
	for (int i = 0; i < 100; i++) {
		emit("OB.", t);
		t += 33;
	}

	/* Sleep a bit to prevent unsorted events */
	while (ovni_clock_now() < t)
		usleep(10);

	instr_end();

	return 0;
}
