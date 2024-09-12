/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "model.h"
#include <string.h>
#include "common.h"
#include "version.h"
#include "emu.h"
#include "emu_args.h"
#include "model_evspec.h"
#include "ev_spec.h"
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

	if (model->registered[i]) {
		err("model %c already registered", i);
		return -1;
	}

	if (spec->version == NULL) {
		err("model %s missing version", spec->name);
		return -1;
	}

	if (spec->evlist == NULL) {
		err("model %s missing evlist", spec->name);
		return -1;
	}

	if (spec->evspec != NULL) {
		err("model %s evspec must be NULL", spec->name);
		return -1;
	}

	spec->evspec = calloc(1, sizeof(struct model_evspec));
	if (spec->evspec == NULL) {
		err("calloc failed:");
		return -1;
	}

	if (model_evspec_init(spec->evspec, spec) != 0) {
		err("model_evspec_init failed for model %s", spec->name);
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
		if (ret > 0 || emu->args.enable_all_models) {
			model->enabled[i] = 1;
			nenabled++;
		}
	}

	if (nenabled == 0) {
		warn("no models enabled");
	} else {
		if (emu->args.enable_all_models)
			info("all %d models are enabled (-a): ", nenabled);
		else
			info("the following %d models are enabled: ", nenabled);
		for (int i = 0; i < MAX_MODELS; i++) {
			if (!model->enabled[i])
				continue;

			struct model_spec *spec = model->spec[i];
			long nevents = spec->evspec->nevents;

			info(" %8s %s '%c' (%ld events)",
					spec->name,
					spec->version,
					(char) i,
					nevents);
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
	struct model_spec *spec = model->spec[index];

	if (!model->registered[index]) {
		err("model not registered for event %s", emu->ev->mcv);
		return -1;
	}

	if (!model->enabled[index]) {
		err("model %s not enabled for event %s",
				spec->name, emu->ev->mcv);
		info("missing call to ovni_thread_require(\"%s\", \"%s\")?",
				spec->name, spec->version);
		return -1;
	}

	if (spec->event == NULL)
		return 0;

	if (spec->event(emu) != 0) {
		err("event() failed for '%s' model", spec->name);
		return -1;
	}

	return 0;
}

int
model_event_print(struct model *model, struct emu_ev *ev,
		char *buf, int buflen)
{
	int index = ev->m;
	if (!model->registered[index]) {
		err("no model registered for %c", ev->m);
		return -1;
	}

	struct model_spec *spec = model->spec[index];
	struct ev_spec *es = model_evspec_find(spec->evspec, ev->mcv);

	if (es == NULL) {
		err("cannot find event definition for %s", ev->mcv);
		return -1;
	}

	if (ev_spec_print(es, ev, buf, buflen) < 0) {
		err("cannot print event signature for %s", ev->mcv);
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

static int
should_enable(int have[3], struct model_spec *spec, struct thread *t)
{
	if (t->meta == NULL) {
		err("missing metadata for thread %s", t->id);
		return -1;
	}

	JSON_Object *require = json_object_dotget_object(t->meta, "ovni.require");
	if (require == NULL) {
		err("missing 'ovni.require' key in thread %s", t->id);
		return -1;
	}

	/* May not have the current model */
	const char *req_version = json_object_get_string(require, spec->name);
	if (req_version == NULL)
		return 0;

	int want[3];
	if (version_parse(req_version, want) != 0) {
		err("cannot parse version %s from thread %s", req_version, t->id);
		return -1;
	}

	if (!version_is_compatible(want, have)) {
		err("unsupported %s model version (want %s, have %s) in %s",
				spec->name, req_version, spec->version,
				t->id);
		return -1;
	}

	/* Compatible */
	return 1;
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
		int ret = should_enable(have, spec, t);

		if (ret < 0) {
			err("cannot determine if model %s must be enabled", spec->name);
			return -1;
		} else if (ret != 0) {
			enable = 1;
			/* Check the rest of threads */
		}
	}

	if (enable) {
		dbg("model %s enabled", spec->name);
	} else {
		dbg("model %s disabled", spec->name);
	}

	return enable;
}
