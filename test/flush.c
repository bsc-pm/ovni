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

// Define gettid for older glibc versions (below 2.30)
#if !__GLIBC_PREREQ(2, 30)
static inline pid_t gettid(void)
{
	return (pid_t)syscall(SYS_gettid);
}
#endif

static inline void
init(void)
{
	char hostname[HOST_NAME_MAX];

	if(gethostname(hostname, HOST_NAME_MAX) != 0)
	{
		perror("gethostname failed");
		exit(EXIT_FAILURE);
	}

	ovni_proc_init(0, hostname, getpid());
	ovni_thread_init(gettid());
	ovni_add_cpu(0, 0);
}

static void emit(uint8_t *buf, size_t size)
{
	struct ovni_ev ev = {0};
	ovni_clock_update();
	ovni_ev_set_mcv(&ev, "O$$");
	ovni_ev_jumbo_emit(&ev, buf, size);
}

int main(void)
{
	size_t payload_size;
	uint8_t *payload_buf;

	init();

	payload_size = (size_t) (0.9 * (double) OVNI_MAX_EV_BUF);
	payload_buf = calloc(1, payload_size);

	if(!payload_buf)
	{
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
