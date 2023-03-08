/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MUX_H
#define MUX_H

#include <stdint.h>
#include "common.h"
#include "value.h"
struct bay;
struct chan;
struct mux;

struct mux_input {
	int64_t index;
	struct chan *chan;
	int selected;
	struct chan *output;
	struct bay_cb *cb;
};

typedef int (* mux_select_func_t)(struct mux *mux,
		struct value value,
		struct mux_input **input);

struct mux {
	struct bay *bay;
	int64_t ninputs;
	int64_t selected;
	struct mux_input *inputs;
	mux_select_func_t select_func;
	struct chan *select;
	struct chan *output;
	struct value def;
};

void mux_input_init(struct mux_input *mux,
		struct value key,
		struct chan *chan);

USE_RET int mux_init(struct mux *mux,
		struct bay *bay,
		struct chan *select,
		struct chan *output,
		mux_select_func_t select_func,
		int64_t ninputs);

USE_RET int mux_set_input(struct mux *mux,
		int64_t index,
		struct chan *input);

USE_RET struct mux_input *mux_get_input(struct mux *mux,
		int64_t index);

USE_RET int mux_register(struct mux *mux,
		struct bay *bay);

void mux_set_default(struct mux *mux, struct value def);

#endif /* MUX_H */
