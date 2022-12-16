/* Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: MIT */

#ifndef COMPAT_H
#define COMPAT_H

#define _GNU_SOURCE
#include <sys/syscall.h>
#include <unistd.h>

/* Define gettid for older glibc versions (below 2.30) */
#if defined(__GLIBC__)
#if !__GLIBC_PREREQ(2, 30)
static inline pid_t
gettid(void)
{
	return (pid_t) syscall(SYS_gettid);
}
#endif /* !__GLIBC_PREREQ(2, 30) */
#endif /* defined(__GLIBC__) */

#endif /* COMPAT_H */
