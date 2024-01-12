/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MODEL_H
#define MODEL_H

#include "common.h"
#include "emu_hook.h"
struct emu;
struct emu_ev;
struct ev_decl;
struct ev_spec;

struct model_spec {
	const char *name;
	const char *version;
	int model;
	struct ev_decl *evlist;
	emu_hook_t *probe;
	emu_hook_t *create;
	emu_hook_t *connect;
	emu_hook_t *event;
	emu_hook_t *finish;

	struct model_evspec *evspec;
};

#define MAX_MODELS 256

struct model {
	struct model_spec *spec[MAX_MODELS];
	int registered[MAX_MODELS];
	int enabled[MAX_MODELS];
};

        void model_init(struct model *model);
USE_RET int model_register(struct model *model, struct model_spec *spec);
USE_RET int model_probe(struct model *model, struct emu *emu);
USE_RET int model_create(struct model *model, struct emu *emu);
USE_RET int model_connect(struct model *model, struct emu *emu);
USE_RET int model_event(struct model *model, struct emu *emu, int index);
USE_RET int model_event_print(struct model *model, struct emu_ev *ev,
		char *buf, int buflen);
USE_RET int model_finish(struct model *model, struct emu *emu);
USE_RET int model_version_probe(struct model_spec *spec, struct emu *emu);

#endif /* MODEL_H */
