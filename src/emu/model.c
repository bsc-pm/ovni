/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

//#define ENABLE_DEBUG

#include "model.h"
#include <string.h>
#include "common.h"
struct emu;

void
model_init(struct model *model)
{
	memset(model, 0, sizeof(struct model));
}

int
model_register(struct model *model, struct model_spec *spec)
{
	int i = spec->model;

	if (model->registered[i]) {
		err("model %c already registered", i);
		return -1;
	}

	model->spec[i] = spec;
	model->registered[i] = 1;

	dbg("registered model %c", (char) i);
	return 0;
}

int
model_probe(struct model *model, struct emu *emu)
{
	for (int i = 0; i < MAX_MODELS; i++) {
		if (!model->registered[i])
			continue;

		struct model_spec *spec = model->spec[i];
		if (spec->probe == NULL)
			continue;

		int ret = spec->probe(emu);
		if (ret < 0) {
			err("probe failed for model '%c'", (char) i);
			return -1;
		}

		if (ret == 0) {
			model->enabled[i] = 1;
			dbg("model %c enabled", (char) i);
		} else {
			dbg("model %c disabled", (char) i);
		}
	}
	return 0;
}

int
model_create(struct model *model, struct emu *emu)
{
	for (int i = 0; i < MAX_MODELS; i++) {
		if (!model->enabled[i])
			continue;

		struct model_spec *spec = model->spec[i];
		if (spec->create == NULL)
			continue;

		if (spec->create(emu) != 0) {
			err("create failed for model '%c'", (char) i);
			return -1;
		}
	}
	return 0;
}

int
model_connect(struct model *model, struct emu *emu)
{
	for (int i = 0; i < MAX_MODELS; i++) {
		if (!model->enabled[i])
			continue;

		struct model_spec *spec = model->spec[i];
		if (spec->connect == NULL)
			continue;

		if (spec->connect(emu) != 0) {
			err("connect failed for model '%c'", (char) i);
			return -1;
		}

		dbg("connect for model %c ok", (char) i);
	}
	return 0;
}

int
model_event(struct model *model, struct emu *emu, int index)
{
	if (!model->registered[index]) {
		err("model not registered for '%c'", (char) index);
		return -1;
	}

	if (!model->enabled[index]) {
		err("model not enabled for '%c'", (char) index);
		return -1;
	}

	struct model_spec *spec = model->spec[index];
	if (spec->event == NULL)
		return 0;

	if (spec->event(emu) != 0) {
		err("event() failed for model '%c'", (char) index);
		return -1;
	}

	return 0;
}

int
model_finish(struct model *model, struct emu *emu)
{
	int ret = 0;
	for (int i = 0; i < MAX_MODELS; i++) {
		if (!model->enabled[i])
			continue;

		struct model_spec *spec = model->spec[i];
		if (spec->finish == NULL)
			continue;

		if (spec->finish(emu) != 0) {
			err("finish failed for model '%c'", (char) i);
			ret = -1;
		}
	}

	return ret;
}
