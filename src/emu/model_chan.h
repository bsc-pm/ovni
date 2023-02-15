/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MODEL_CHAN_H
#define MODEL_CHAN_H

#include "model_pvt.h"

struct model_chan_spec {
	int nch;
	const char *prefix;
	const char **ch_names;
	const int *ch_stack;
	const struct model_pvt_spec *pvt;
	const int *track;
};

#endif /* MODEL_CHAN_H */
