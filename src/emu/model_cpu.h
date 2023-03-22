/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MODEL_CPU_H
#define MODEL_CPU_H

#include <stddef.h>
#include "common.h"
struct emu;

struct model_cpu_spec {
	size_t size;
	const struct model_chan_spec *chan;
	const struct model_spec *model;
};

struct model_cpu {
	const struct model_cpu_spec *spec;
	struct bay *bay;
	struct track *track;
};

USE_RET int model_cpu_create(struct emu *emu, const struct model_cpu_spec *spec);
USE_RET int model_cpu_connect(struct emu *emu, const struct model_cpu_spec *spec);

#endif /* MODEL_CPU_H */
