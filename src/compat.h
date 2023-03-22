/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: MIT */

#ifndef COMPAT_H
#define COMPAT_H

#include <time.h>

pid_t get_tid(void);
int sleep_us(long usec);

#endif /* COMPAT_H */
