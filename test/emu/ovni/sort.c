/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
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
emit_jumbo(uint8_t *buf, size_t size, int64_t clock)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, "OUj");
	ovni_ev_set_clock(&ev, clock);
	ovni_ev_jumbo_emit(&ev, buf, size);
}

static void
emit(char *mcv, int64_t clock)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, mcv);
	ovni_ev_set_clock(&ev, clock);
	ovni_ev_emit(&ev);
}

#define BUFSIZE 128

int
main(void)
{
	uint8_t buf[BUFSIZE];
	instr_start(0, 1);

	/* Leave some room to prevent clashes */
	usleep(100); /* 100000 us */

	int64_t t0 = ovni_clock_now();

	emit("OU[", t0);

	int64_t t = t0 - 10000;
	for (int i = 0; i < 100; i++) {
		emit("OB.", t);
		t += 33;
	}

	/* Also test jumbo events */
	for (int i = 0; i < BUFSIZE; i++)
		buf[i] = i & 0xff;

	emit_jumbo(buf, BUFSIZE, t);

	/* Sleep a bit to prevent early OU] */
	while ((int64_t) ovni_clock_now() < t)
		usleep(10);

	emit("OU]", ovni_clock_now());

	instr_end();

	return 0;
}
