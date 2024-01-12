/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EV_SPEC_H
#define EV_SPEC_H

#include <inttypes.h>
#include <stddef.h>
#include "uthash.h"

struct ev_decl {
	const char *signature;
	const char *description;
};

enum ev_arg_type {
	U8 = 0,
	U16,
	U32,
	U64,
	I8,
	I16,
	I32,
	I64,
	STR,
	MAX_TYPE
};

#define MAX_ARGS 16

struct ev_arg {
	size_t size; /* in bytes */
	size_t offset; /* in bytes */
	enum ev_arg_type type;
	char name[64];
};

struct ev_spec {
	char mcv[4];
	char signature[256];
	int is_jumbo;
	int nargs;
	struct ev_arg args[MAX_ARGS];
	size_t payload_size;
	const char *description;

	UT_hash_handle hh; /* hash map by MCV for model_evspec */
};

/* Helpers for event pairs (with same with). */
#define PAIR_E(MCV1, MCV2, desc) \
	{ MCV1, "enters " desc }, \
	{ MCV2, "leaves " desc },

#define PAIR_B(MCV1, MCV2, desc) \
	{ MCV1, "begins " desc }, \
	{ MCV2, "ceases " desc },

#define PAIR_S(MCV1, MCV2, desc) \
	{ MCV1, "starts " desc }, \
	{ MCV2, "stops  " desc },

struct emu_ev;

int ev_spec_compile(struct ev_spec *spec, struct ev_decl *decl);
int ev_spec_print(struct ev_spec *spec, struct emu_ev *ev, char *outbuf, int outlen);
struct ev_arg *ev_spec_find_arg(struct ev_spec *spec, const char *name);

#endif /* EV_SPEC_H */
