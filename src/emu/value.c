/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "value.h"

char value_buffers[VALUE_NBUF][VALUE_BUFSIZE];
size_t value_nextbuf = 0;
