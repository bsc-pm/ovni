#ifndef MUX_H
#define MUX_H

#include <stdint.h>
#include "bay.h"
#include "uthash.h"

struct mux_input {
	struct value key;
	struct chan *chan;
	UT_hash_handle hh;
};

struct mux;

typedef int (* mux_select_func_t)(struct mux *mux,
		struct value value,
		struct mux_input **input);

struct mux {
	struct bay *bay;
	int64_t ninputs;
	struct mux_input *input;
	mux_select_func_t select_func;
	struct chan *select;
	struct chan *output;
};

void mux_input_init(struct mux_input *mux,
		struct value key,
		struct chan *chan);

int mux_init(struct mux *mux,
		struct bay *bay,
		struct chan *select,
		struct chan *output,
		mux_select_func_t select_func);

struct mux_input *mux_find_input(struct mux *mux,
		struct value key);

int mux_add_input(struct mux *mux,
		struct value key,
		struct chan *input);

int mux_register(struct mux *mux,
		struct bay *bay);

#endif /* MUX_H */
