/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ev_spec.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "emu_ev.h"
#include "ovni.h"

static const char *type_name[MAX_TYPE] = {
	[U8]  = "u8",
	[U16] = "u16",
	[U32] = "u32",
	[U64] = "u64",
	[I8]  = "i8",
	[I16] = "i16",
	[I32] = "i32",
	[I64] = "i64",
	[STR] = "str",
};

static const char *type_fmt[MAX_TYPE] = {
	[U8]  = "%" PRIu8,
	[U16] = "%" PRIu16,
	[U32] = "%" PRIu32,
	[U64] = "%" PRIu64,
	[I8]  = "%" PRId8,
	[I16] = "%" PRId16,
	[I32] = "%" PRId32,
	[I64] = "%" PRId64,
	[STR] = "%s",
};

static size_t type_size[MAX_TYPE] = {
	[U8]  = 1,
	[U16] = 2,
	[U32] = 4,
	[U64] = 8,
	[I8]  = 1,
	[I16] = 2,
	[I32] = 4,
	[I64] = 8,
	[STR] = 0, /* Has to be computed */
};

struct cursor {
	const char *in; /* Pointer to next char in input buffer */
	char *out; /* Pointer to next char in output buffer */
	int len; /* Remaining size in output buffer */
};

static void
advance_out(struct cursor *c, int n)
{
	/* Advance buffer and update len */
	c->out += n;
	c->len -= n;
}

static void
advance_in(struct cursor *c, int n)
{
	c->in += n;
}

static int
parse_type(struct ev_arg *argspec, char *type)
{
	for (int i = 0; i < MAX_TYPE; i++) {
		if (strcmp(type, type_name[i]) == 0) {
			argspec->type = (enum ev_arg_type) i;
			return 0;
		}
	}

	err("cannot find matching type for '%s'", type);
	return -1;
}

static int
parse_arg(struct ev_spec *spec, char *arg)
{
	if (spec->nargs >= MAX_ARGS) {
		err("too many arguments");
		return -1;
	}

	struct ev_arg *argspec = &spec->args[spec->nargs];

	char *saveptr = NULL;
	char *type = strtok_r(arg, " ", &saveptr);
	if (type == NULL) {
		err("cannot parse type in argument '%s'", arg);
		return -1;
	}

	char *name = strtok_r(NULL, " ", &saveptr);
	if (name == NULL) {
		err("cannot parse name in argument '%s'", arg);
		return -1;
	}

	/* Copy name */
	size_t n = (size_t) snprintf(argspec->name, sizeof(argspec->name), "%s", name);
	if (n >= sizeof(argspec->name)) {
		err("argument name too long: %s", name);
		return -1;
	}

	if (parse_type(argspec, type) != 0) {
		err("cannot determine type in argument '%s'", arg);
		return -1;
	}

	argspec->size = type_size[argspec->type];
	argspec->offset = spec->payload_size;
	spec->nargs++;
	spec->payload_size += argspec->size;

	return 0;
}

static int
parse_args(struct ev_spec *spec, char *paren)
{
	paren++;

	/* Skip jumbo size */
	if (spec->is_jumbo)
		spec->payload_size = 4;
	else
		spec->payload_size = 0;

	char *saveptr = NULL;
	char *arg = strtok_r(paren, ",)", &saveptr);
	while (arg) {
		if (parse_arg(spec, arg) != 0) {
			err("cannot parse argument '%s'", arg);
			return -1;
		}

		arg = strtok_r(NULL, ",)", &saveptr);
	}

	return 0;
}

static int
is_mcv_valid(char m, char c, char v)
{
	return isgraph(m) && isgraph(c) && isgraph(v);
}

static int
parse_signature(struct ev_spec *spec, char *sig)
{
	if (strlen(sig) < 3) {
		err("signature too short: %s", sig);
		return -1;
	}

	char M = sig[0];
	char C = sig[1];
	char V = sig[2];

	/* The MCV part must be printable */
	if (!is_mcv_valid(M, C, V)) {
		err("invalid MCV: %s", sig);
		return -1;
	}

	spec->mcv[0] = M;
	spec->mcv[1] = C;
	spec->mcv[2] = V;
	spec->mcv[3] = '\0';

	/* The next character may be '+' if jumbo */
	char *next = &sig[3];
	if (*next == '+') {
		spec->is_jumbo = 1;
		next++;
	}

	/* No arguments */
	if (*next == '\0') {
		if (spec->is_jumbo) {
			err("missing jumbo arguments in signature: %s", sig);
			return -1;
		}

		return 0;
	}

	/* If there are arguments, it must have one parenthesis */
	if (*next != '(') {
		err("expecting parenthesis '(' for arguments: %s", sig);
		return -1;
	}

	/* Place args pointer to the first parenthesis '(' */
	char *args = next;

	if (parse_args(spec, args) < 0) {
		err("cannot parse arguments: %s", sig);
		return -1;
	}

	/* Must have at least one argument */
	if (spec->nargs == 0) {
		err("empty arguments: %s", sig);
		return -1;
	}

	return 0;
}

int
ev_spec_compile(struct ev_spec *spec, struct ev_decl *decl)
{
	memset(spec, 0, sizeof(struct ev_spec));

	/* Working copy so we can modify it */
	char sig[256];

	if (snprintf(sig, 256, "%s", decl->signature) >= 256) {
		err("signature too long: %s", decl->signature);
		return -1;
	}

	int ret = parse_signature(spec, sig);

	if (ret != 0) {
		err("cannot parse signature '%s'", sig);
		return -1;
	}

	spec->description = decl->description;

	return 0;
}

struct ev_arg *
ev_spec_find_arg(struct ev_spec *spec, const char *name)
{
	for (int i = 0; i < spec->nargs; i++) {
		struct ev_arg *arg = &spec->args[i];
		if (strcmp(arg->name, name) == 0)
			return arg;
	}

	return NULL;
}

/* Parse printf format specifier like:
 * %3d{cpu}
 *  |
 *  c->in points to the next char after the %
 *
 *  Precondition: *c->in != '{'.
 * */
static int
parse_printf_format(char *fmt, int buflen, struct cursor *c)
{
	int n = buflen - 1;
	int ifmt = 0;

	/* Check precondition */
	if (*c->in == '{') {
		err("missing format");
		return -1;
	}

	if (n < 1) {
		err("no space for arg name");
		return -1;
	}

	if (ifmt >= n) {
		err("buffer empty");
		return -1;
	}

	/* Always write the first % */
	fmt[ifmt++] = '%';

	for (; *c->in != '{'; c->in++) {
		if (*c->in == '\0') {
			err("unexpected end of format");
			return -1;
		}
		if (ifmt >= n) {
			err("format too long");
			return -1;
		}
		fmt[ifmt++] = *c->in;
	}

	/* Complete the printf format */
	fmt[ifmt] = '\0';

	return 0;
}

/* Parse argument name specifier like:
 * %3d{cpu}
 *     |
 *     c->in points to the next char after the {
 *
 *  Precondition: *c->in != '}'.
 * */
static int
parse_arg_name(char *arg, int buflen, struct cursor *c)
{
	int n = buflen - 1;
	int iarg = 0;

	/* Check precondition */
	if (*c->in == '}') {
		err("missing argument name");
		return -1;
	}

	if (n < 1) {
		err("no space for arg name");
		return -1;
	}

	if (iarg >= n) {
		err("buffer empty");
		return -1;
	}

	/* Parse argument name */
	for (; *c->in != '}'; c->in++) {
		if (*c->in == '\0') {
			err("unexpected end of argument name");
			return -1;
		}
		if (!isalnum(*c->in)) {
			err("bad argument name");
			return -1;
		}
		if (iarg >= n) {
			err("argument name too long");
			return -1;
		}
		arg[iarg++] = *c->in;
	}

	arg[iarg] = '\0';

	return 0;
}

static int
print_arg(struct ev_arg *arg, const char *fmt, struct cursor *c, struct emu_ev *ev)
{
	int n = 0;
	uint8_t *payload = (uint8_t *) ev->payload;

#define CASE(TYPE) \
		do { \
			TYPE data; \
			memcpy(&data, &payload[arg->offset], sizeof(data)); \
			n = snprintf(c->out, (size_t) c->len, fmt, data); \
			if (n >= c->len) { \
				err("no space for argument"); \
				return -1; \
			} \
			advance_out(c, n); \
		} while (0); break;

	switch (arg->type) {
		case U8:  CASE(uint8_t);
		case U16: CASE(uint16_t);
		case U32: CASE(uint32_t);
		case U64: CASE(uint64_t);
		case I8:  CASE(int8_t);
		case I16: CASE(int16_t);
		case I32: CASE(int32_t);
		case I64: CASE(int64_t);
		case STR:
			{
				char *data = (char *) &payload[arg->offset];
				/* Here we trust the input string to
				 * contain a nil at the end */
				int n = snprintf(c->out, (size_t) c->len, fmt, data);
				if (n >= (int) c->len) {
					err("no space for string argument");
					return -1;
				}
				advance_out(c, n);
				break;
			}
		default:
			  err("bad type");
			  return -1;
	}

#undef CASE

	return 0;
}

/* Returns 0 on success or -1 on error. The input and output pointers
 * are advanced accordingly. */
static int
format_region(struct ev_spec *spec, struct cursor *c, struct emu_ev *ev)
{
	if (c->len == 0) {
		err("no more room");
		return -1;
	}

	/* Begins with percent pointing to %{xxx} */
	if (*c->in != '%') {
		err("expecting initial %%");
		return -1;
	}

	advance_in(c, 1); /* Skip initial % */

	/* If the string ends just there is an error, like "xxx %" */
	if (*c->in == '\0') {
		err("truncated '%%' format");
		return -1;
	}

	/* At least len == 1, no check needed here */
	if (*c->in == '%') {
		*c->out = '%';
		advance_out(c, 1);
		advance_in(c, 1); /* Eat the second % in the input */
		return 0;
	}

	int infer_fmt = 0;
	char fmt[64];

	if (*c->in == '{') {
		/* Missing format, use default inferred from the type
		 * later */
		infer_fmt = 1;
	} else {
		if (parse_printf_format(fmt, sizeof(fmt), c) != 0) {
			err("cannot parse printf format");
			return -1;
		}
	}

	if (*c->in != '{') {
		err("expecting opening bracket");
		return -1;
	}

	advance_in(c, 1); /* Skip opening braket */

	char argname[64];

	if (parse_arg_name(argname, sizeof(argname), c) != 0) {
		err("cannot parse argument name");
		return -1;
	}

	if (*c->in != '}') {
		err("expecting closing bracket");
		return -1;
	}

	advance_in(c, 1); /* Skip closing braket */

	/* Find argument by name in spec */
	struct ev_arg *arg = ev_spec_find_arg(spec, argname);

	if (arg == NULL) {
		err("cannot find argument %s", argname);
		return -1;
	}

	/* If there was no custom format, use the default */
	if (infer_fmt) {
		if (snprintf(fmt, sizeof(fmt), "%s", type_fmt[arg->type]) >= 64) {
			err("format type too long");
			return -1;
		}
	}

	if (print_arg(arg, fmt, c, ev) != 0) {
		err("cannot print argument %s", argname);
		return -1;
	}

	return 0;
}

int
ev_spec_print(struct ev_spec *spec, struct emu_ev *ev, char *outbuf, int outlen)
{
	if (outlen <= 0) {
		err("buffer has no space");
		return -1;
	}

	struct cursor c = {
		.in = spec->description,
		.out = outbuf,
		.len = outlen - 1, /* Leave room for the nil */
	};

	/* Invariant len >= 0, so the nil character always fits. */
	while (*c.in != '\0') {
		if (c.len == 0) {
			err("description too long for buffer");
			return -1;
		}

		if (*c.in == '%') {
			/* Begin format region */
			if (format_region(spec, &c, ev) != 0) {
				err("format_region failed");
				return -1;
			}
		} else {
			/* Just copy the character in the output */
			*c.out = *c.in;
			c.in++;
			advance_out(&c, 1);
		}
	}

	/* Finish the buffer */
	*c.out = '\0';

	return 0;
}
