/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MODELS_H
#define MODELS_H

#include "common.h"
struct model;

USE_RET int models_register(struct model *model);

#endif /* MODELS_H */
