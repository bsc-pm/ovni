/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "models.h"

#include "common.h"
#include "model.h"
#include <stdlib.h>
#include <string.h>

extern struct model_spec model_ovni;
extern struct model_spec model_nanos6;
extern struct model_spec model_nosv;
extern struct model_spec model_nodes;
extern struct model_spec model_tampi;
extern struct model_spec model_mpi;
extern struct model_spec model_kernel;
extern struct model_spec model_openmp;

static struct model_spec *models[] = {
	&model_ovni,
	&model_nanos6,
	&model_nosv,
	&model_nodes,
	&model_tampi,
	&model_mpi,
	&model_kernel,
	&model_openmp,
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

const char *
models_get_version(const char *name)
{
	for (int i = 0; models[i] != NULL; i++) {
		if (strcmp(models[i]->name, name) == 0)
			return models[i]->version;
	}

	return NULL;
}

void
models_print(void)
{
	for (int i = 0; models[i] != NULL; i++) {
		struct model_spec *spec = models[i];
		rerr("  %c  %-8s  %s\n", spec->model, spec->name, spec->version);
	}
}
