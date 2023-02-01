/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "models.h"

#include <stdlib.h>

extern struct model_spec model_ovni;
extern struct model_spec model_nanos6;

static struct model_spec *models[] = {
	&model_ovni,
	&model_nanos6,
	NULL
};

void
models_register(struct model *model)
{
	for (int i = 0; models[i] != NULL; i++) {
		model_register(model, models[i]);
	}
}
