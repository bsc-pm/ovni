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
emit(uint8_t *buf, size_t size)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, "O$$");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_jumbo_emit(&ev, buf, size);
}

int
main(void)
{
	size_t payload_size;
	uint8_t *payload_buf;

	init();

	payload_size = (size_t) (0.9 * (double) OVNI_MAX_EV_BUF);
	payload_buf = calloc(1, payload_size);

	if (!payload_buf) {
		perror("calloc failed");
		exit(EXIT_FAILURE);
	}

	/* The first should fit, filling 90 % of the buffer */
	emit(payload_buf, payload_size);

	/* The second event would cause a flush */
	emit(payload_buf, payload_size);

	/* The third would cause the previous flush events to be written
	 * to disk */
	emit(payload_buf, payload_size);

	/* Flush the last event to disk manually */
	ovni_flush();
	ovni_proc_fini();

	free(payload_buf);

	return 0;
}
