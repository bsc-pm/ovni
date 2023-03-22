/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MODEL_THREAD_H
#define MODEL_THREAD_H

#include <stddef.h>
#include "common.h"
struct emu;

struct model_thread_spec {
	size_t size;
	const struct model_chan_spec *chan;
	const struct model_spec *model;
};

struct model_thread {
	const struct model_thread_spec *spec;
	struct bay *bay;
	struct chan *ch;
	struct track *track;
};

USE_RET int model_thread_create(struct emu *emu, const struct model_thread_spec *spec);
USE_RET int model_thread_connect(struct emu *emu, const struct model_thread_spec *spec);

#endif /* MODEL_THREAD_H */
