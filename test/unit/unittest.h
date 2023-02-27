/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef UNITTEST_H
#define UNITTEST_H

#include "common.h"

#define OK(x) do { if ((x) != 0) { die(#x " failed"); } } while (0)
#define ERR(x) do { if ((x) == 0) { die(#x " didn't fail"); } } while (0)

#endif /* UNITTEST_H */
