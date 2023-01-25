/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu_model.h"

#include <stdlib.h>

int
emu_model_register(struct emu_model *model, struct model_spec *spec, void *ctx)
{
	int i = spec->model;
	model->ctx[i] = ctx;
	model->spec[i] = spec;
	return 0;
}

void *
emu_model_get_context(struct emu_model *model, struct model_spec *spec, int imodel)
{
	for (int i = 0; spec->depends[i]; i++) {
		if (spec->depends[i] == imodel)
			return model->ctx[imodel];
	}

	/* Not allowed */
	return NULL;
}
