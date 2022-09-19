/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <limits.h>

#include "ovni.h"
#include "compat.h"

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
	ovni_ev_set_mcv(&ev, "O$$");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_jumbo_emit(&ev, buf, size);
}

#define NRUNS 50

int main(void)
{
	size_t payload_size;
	uint8_t *payload_buf;

	if(setenv("OVNI_TMPDIR", "/dev/shm/ovni-flush-overhead", 1) != 0)
	{
		perror("setenv failed");
		exit(EXIT_FAILURE);
	}

	init();

	/* Use 1 MiB of payload */
	payload_size = 1024 * 1024;
	payload_buf = calloc(1, payload_size);

	if(!payload_buf)
	{
		perror("calloc failed");
		exit(EXIT_FAILURE);
	}

	double *times = calloc(sizeof(double), NRUNS);
	if(times == NULL)
	{
		perror("calloc failed");
		exit(EXIT_FAILURE);
	}

	for(int i=0; i < NRUNS; i++)
	{
		emit(payload_buf, payload_size);
		double t0 = (double) ovni_clock_now();
		ovni_flush();
		double t1 = (double) ovni_clock_now();

		times[i] = (t1 - t0) * 1e-6; /* to milliseconds */
	}

	double mean = 0.0;
	for(int i=0; i < NRUNS; i++)
		mean += times[i];
	mean /= (double) NRUNS;
	
	fprintf(stderr, "mean %f ms\n", mean);

	/* It should be able to write 1 MiB in less than 1 ms */
	if (mean > 1.0) {
		fprintf(stderr, "took too much time to flush: %f ms\n", mean);

		fprintf(stderr, "times (in ms):\n");
		for(int i=0; i < NRUNS; i++)
		    fprintf(stderr, " %4d  %f\n", i, times[i]);

		exit(EXIT_FAILURE);
	}

	ovni_proc_fini();

	free(payload_buf);
	free(times);

	return 0;
}
