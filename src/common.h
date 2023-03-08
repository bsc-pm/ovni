/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: MIT */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>

extern int is_debug_enabled;

/* Debug macros */

void progname_set(char *name);
void enable_debug(void);
void verr(const char *prefix, const char *func, const char *errstr, ...);
void vdie(const char *prefix, const char *func, const char *errstr, ...);

/* clang-format off */

#define rerr(...) fprintf(stderr, __VA_ARGS__)
#define err(...)  verr("ERROR", __func__, __VA_ARGS__)
#define die(...)  vdie("FATAL", __func__, __VA_ARGS__)
#define info(...) verr("INFO", NULL, __VA_ARGS__)
#define warn(...) verr("WARN", NULL, __VA_ARGS__)

#define dbg(...) do { \
	if (unlikely(is_debug_enabled)) verr("DEBUG", __func__, __VA_ARGS__); \
} while (0);

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define UNUSED(x) (void)(x)

/* Poison assert */
#pragma GCC poison assert

#define USE_RET __attribute__((warn_unused_result))

#define ARRAYLEN(x) (sizeof(x)/sizeof((x)[0]))

/* clang-format on */



#endif /* COMMON_H */
