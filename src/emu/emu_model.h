/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_MODEL_H
#define EMU_MODEL_H

typedef int (emu_hook_t)(void *ptr);

struct model_spec {
	char *name;
	int model;
	char *depends;
	emu_hook_t *probe;
	emu_hook_t *create;
	emu_hook_t *connect;
	emu_hook_t *event;
};

struct emu_model {
	struct model_spec *spec[256];
	void *ctx[256];
};

int emu_model_parse(struct emu_model *model, struct model_spec *spec, void *ctx);
int emu_model_register(struct emu_model *model, struct model_spec *spec, void *ctx);
int emu_model_register(struct emu_model *model, struct model_spec *spec, void *ctx);
void *emu_model_get_context(struct emu_model *model, struct model_spec *spec, int imodel);

#endif /* EMU_MODEL_H */
