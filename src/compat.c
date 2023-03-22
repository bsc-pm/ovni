/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: MIT */

#define _GNU_SOURCE /* Only here */

#include "compat.h"
#include <unistd.h>
#include <errno.h>

/* Define gettid for older glibc versions (below 2.30) */
#if defined(__GLIBC__)
#if !__GLIBC_PREREQ(2, 30)

#include <sys/syscall.h>

static pid_t
gettid(void)
{
	return (pid_t) syscall(SYS_gettid);
}

#endif /* !__GLIBC_PREREQ(2, 30) */
#endif /* defined(__GLIBC__) */

pid_t
get_tid(void)
{
	return gettid();
}

int
sleep_us(long usec)
{
	struct timespec ts;
	int res;

	if (usec < 0) {
		errno = EINVAL;
		return -1;
	}

	ts.tv_sec = usec / 1000000L;
	ts.tv_nsec = (usec % 1000000L) * 1000L;

	do {
		res = nanosleep(&ts, &ts);
	} while (res && errno == EINTR);

	return res;
}
