/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "common.h"
#include "compat.h"
#include <stdio.h>
#include <stdlib.h>

static long
fib(long index)
{
	long a, b;
	if (index <= 1)
		return index;
	#pragma oss task shared(a) label("fibonacci")
	a = fib(index-1);
	#pragma oss task shared(b) label("fibonacci")
	b = fib(index-2);
	#pragma oss taskwait
	return a + b;
}

/* Outline task version */
#pragma oss task label("fibonacci")
static void
fib2(long index, long *res)
{
	if (index <= 1) {
		*res = index;
	} else {
		long a, b;
		fib2(index-1, &a);
		fib2(index-2, &b);
		#pragma oss taskwait
		*res = a + b;
	}
}

int
main(void)
{
	FILE *f = fopen("/proc/sys/kernel/perf_event_paranoid", "r");
	if (f == NULL)
		die("cannot open /proc/sys/kernel/perf_event_paranoid:");

	char buf[16] = {0};
	if (fread(buf, 1, 16, f) <= 0)
		die("cannot read /proc/sys/kernel/perf_event_paranoid:");

	fclose(f);

	/* We need the value 1 or less:
	 * Access to performance monitoring and observability operations is limited.
	 * Consider adjusting /proc/sys/kernel/perf_event_paranoid setting to open
	 * access to performance monitoring and observability operations for processes
	 * without CAP_PERFMON, CAP_SYS_PTRACE or CAP_SYS_ADMIN Linux capability.
	 * More information can be found at 'Perf events and tool security' document:
	 * https://www.kernel.org/doc/html/latest/admin-guide/perf-security.html
	 * perf_event_paranoid setting is 3:
	 *   -1: Allow use of (almost) all events by all users
	 *       Ignore mlock limit after perf_event_mlock_kb without CAP_IPC_LOCK
	 * >= 0: Disallow raw and ftrace function tracepoint access
	 * >= 1: Disallow CPU event access
	 * >= 2: Disallow kernel profiling
	 * To make the adjusted perf_event_paranoid setting permanent preserve it
	 * in /etc/sysctl.conf (e.g. kernel.perf_event_paranoid = <setting>)
	 */
	int level = atoi(buf);
	if (level > 1)
		die("requires perf_event_paranoid = 1 or less");

	for (int i = 0; i < 30; i++)
		fib(5);

	#pragma oss taskwait

	long res = 0;
	for (int i = 0; i < 30; i++) {
		fib2(5, &res);
		#pragma oss taskwait
	}

	#pragma oss taskwait
	return 0;
}
