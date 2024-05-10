/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MODEL_EVSPEC_H
#define MODEL_EVSPEC_H

#include "common.h"

struct model_spec;
struct ev_spec;

struct model_evspec {
	/* Hash table indexed by MCV */
	struct ev_spec *spec;
	long nevents;

	/* Contiguous memory for allocated table */
	struct ev_spec *alloc;
};

USE_RET int model_evspec_init(struct model_evspec *evspec, struct model_spec *spec);
USE_RET struct ev_spec *model_evspec_find(struct model_evspec *evspec, char *mcv);

#endif /* MODEL_EVSPEC_H */
