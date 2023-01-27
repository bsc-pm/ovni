/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef UST_MODEL_H
#define UST_MODEL_H

/* The user-space thread "ust" execution model tracks the state of processes and
 * threads running in the CPUs by instrumenting the threads before and after
 * they are going to sleep. It just provides an approximate view of the real
 * execution by the kernel. */

#include "emu_model.h"

extern struct model_spec ust_model_spec;

#endif /* UST_MODEL_H */
