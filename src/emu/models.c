/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "models.h"

#include "common.h"
#include <stdlib.h>

extern struct model_spec model_ovni;
extern struct model_spec model_nanos6;
extern struct model_spec model_nosv;
extern struct model_spec model_nodes;
extern struct model_spec model_kernel;

static struct model_spec *models[] = {
	&model_ovni,
	&model_nanos6,
	&model_nosv,
	&model_nodes,
	&model_kernel,
	NULL
};

int
models_register(struct model *model)
{
	for (int i = 0; models[i] != NULL; i++) {
		if (model_register(model, models[i]) != 0) {
			err("model_register failed");
			return -1;
		}
	}

	return 0;
}
