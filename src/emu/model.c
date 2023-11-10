/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "model.h"
#include <string.h>
#include "common.h"
#include "version.h"
#include "emu.h"
#include "thread.h"
#include "proc.h"

void
model_init(struct model *model)
{
	memset(model, 0, sizeof(struct model));
}

int
model_register(struct model *model, struct model_spec *spec)
{
	int i = spec->model;

	if (spec->name == NULL) {
		err("model %c missing name", i);
		return -1;
	}

	if (spec->version == NULL) {
		err("model %c missing version", i);
		return -1;
	}

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
	int nenabled = 0;
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

		/* Zero is disabled */
		if (ret > 0) {
			model->enabled[i] = 1;
			nenabled++;
		}

		const char *state = model->enabled[i] ? "enabled" : "disabled";
		info("model %8s %s '%c' %s",
				spec->name, spec->version, (char) i, state);
	}

	if (nenabled == 0)
		warn("no models enabled");

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

int
model_version_probe(struct model_spec *spec, struct emu *emu)
{
	int enable = 0;
	int have[3];

	if (version_parse(spec->version, have) != 0) {
		err("cannot parse version %s", spec->version);
		return -1;
	}

	char path[128];
	if (snprintf(path, 128, "%s.version", spec->name) >= 128)
		die("model %s name too long", spec->name);

	/* Find a stream with an model name key */
	struct system *sys = &emu->system;
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		/* If there is not metadata, it may be a stream created with
		 * older libovni, so ensure the proc metadata version. */
		if (t->meta == NULL) {
			/* Version 1 doesn't have thread metadata, so we enable
			 * all models. */
			if (t->proc->metadata_version == 1) {
				warn("found old metadata (version 1), enabling model %s",
						spec->name);
				enable = 1;
				break;
			}

			/* Otherwise is an error */
			err("missing metadata for thread %s with version > 1", t->id);
			return -1;
		}

		JSON_Object *require = json_object_dotget_object(t->meta, "ovni.require");
		if (require == NULL) {
			warn("missing 'ovni.require' key, enabling all models");
			enable = 1;
			break;
		}

		const char *req_version = json_object_get_string(require, spec->name);
		if (req_version == NULL)
			continue;

		int want[3];
		if (version_parse(req_version, want) != 0) {
			err("cannot parse version %s", req_version);
			return -1;
		}

		if (!version_is_compatible(want, have)) {
			err("unsupported %s model version (want %s, have %s) in %s",
					spec->name, req_version, spec->version,
					t->id);
			return -1;
		}

		enable = 1;
	}

	if (enable) {
		dbg("model %s enabled", spec->name);
	} else {
		dbg("model %s disabled", spec->name);
	}

	return enable;
}
