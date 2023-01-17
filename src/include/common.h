/* Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: MIT */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>

/* Debug macros */

/* clang-format off */

#ifdef ENABLE_DEBUG
# define dbg(...) fprintf(stderr, __VA_ARGS__);
#else
# define dbg(...)
#endif

#define err(...) fprintf(stderr, __VA_ARGS__);
#define die(...) do { err("fatal: " __VA_ARGS__); abort(); } while (0)

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define UNUSED(x) (void)(x)

/* Poison assert */
#pragma GCC poison assert

#define USE_RET __attribute__((warn_unused_result))

#define ARRAYLEN(x) (sizeof(x)/sizeof((x)[0]))

/* clang-format on */

#endif /* COMMON_H */
