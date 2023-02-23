/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: MIT */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>

/* Debug macros */

void progname_set(char *name);
void verr(const char *func, const char *errstr, ...);
void vdie(const char *func, const char *errstr, ...);

/* clang-format off */

#define rerr(...) fprintf(stderr, __VA_ARGS__)
#define err(...) verr(__func__, __VA_ARGS__)
#define die(...) vdie(__func__, __VA_ARGS__)
#define info(...) verr("INFO", __VA_ARGS__)
#define warn(...) verr("WARN", __VA_ARGS__)

#ifdef ENABLE_DEBUG
# define dbg(...) verr(__func__, __VA_ARGS__)
#else
# define dbg(...) do { if (0) { verr(__func__, __VA_ARGS__); } } while(0)
#endif

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define UNUSED(x) (void)(x)

/* Poison assert */
#pragma GCC poison assert

#define USE_RET __attribute__((warn_unused_result))

#define ARRAYLEN(x) (sizeof(x)/sizeof((x)[0]))

/* clang-format on */



#endif /* COMMON_H */
