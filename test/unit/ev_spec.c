/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ev_spec.h"

#include "unittest.h"
#include "emu_ev.h"
#include <string.h>

struct testcase {
	struct ev_decl decl;
	uint8_t payload[256];
	int ret_compile;
	int ret_print;
	char *output;
};

static void
test_format(void)
{
	struct testcase cases[] = {
/* Bad compile tests */
		{
			/* Test bad MCV */
			.decl = { "O x", "" },
			.ret_compile = -1,
		},
		{
			/* Jumbo but no argument (nonsense) */
			.decl = { "OHx+", "" },
			.ret_compile = -1,
		},
		{
			/* Empty arguments */
			.decl = { "OHx()", "" },
			.ret_compile = -1,
		},
		{
			/* Missing opening parenthesis */
			.decl = { "OHx)", "" },
			.ret_compile = -1,
		},
		{
			/* Typo, 4 MCV char */
			.decl = { "OOHx", "" },
			.ret_compile = -1,
		},
		{
			/* Typo, 2 MCV char */
			.decl = { "Ox", "" },
			.ret_compile = -1,
		},
		{
			/* Typo, 2 MCV char with args */
			.decl = { "Ox(i32 cpu)", "" },
			.ret_compile = -1,
		},
/* Bad printing tests */
		{
			/* Test missing argument */
			.decl = { "OHx", "hi missing %{cpu}" },
			.ret_print = -1,
		},
		{
			/* Test missing closing bracket */
			.decl = { "OHx(i32 cpu)", "hi missing %{cpu" },
			.ret_print = -1,
		},
		{
			/* Test using parenthesis instead */
			.decl = { "OHx(i32 cpu)", "hi missing %(cpu)" },
			.ret_print = -1,
		},
/* Good tests */
		{
			/* Test arguments in normal event */
			.decl = {
				"OAr(i32 cpu, i32 tid)",
				"changes the affinity of thread %{tid} to CPU %{cpu}"
			},
			.payload = {
				0x01, 0x00, 0x00, 0x00, /* CPU */
				0x02, 0x00, 0x00, 0x00, /* TID */
			},
			.output = "changes the affinity of thread 2 to CPU 1",
		},
		{
			/* Test custom printf format */
			.decl = {
				"OAr(i32 cpu)",
				"we like the CPU %08d{cpu} well padded"
			},
			.payload = {
				0x03, 0x00, 0x00, 0x00, /* CPU */
			},
			.output = "we like the CPU 00000003 well padded",
		},
		{
			/* Test i32 in jumbo */
			.decl = {
				"ooo+(i32 cpu)", "welcome to CPU %{cpu}"
			},
			.payload = {
				0x00, 0x00, 0x00, 0x00, /* jumbo size,
							   ignored */
				0x05, 0x00, 0x00, 0x00, /* cpu */
			},
			.output = "welcome to CPU 5",
		},
		{
			/* Test string in jumbo */
			.decl = {
				"ooo+(str name)", "welcome %{name}!"
			},
			.payload = {
				0x00, 0x00, 0x00, 0x00, /* jumbo size,
							   ignored */
				'a', 'l', 'i', 'e', 'n', '\0', /* name */
			},
			.output = "welcome alien!",
		},
	};

	char buf[1024];
	int bufsize = 1024;
	int n = sizeof(cases) / sizeof(cases[0]);
	for (int i = 0; i < n; i++) {
		struct testcase *c = &cases[i];
		struct ev_spec spec = {0};
		struct ev_decl *decl = &c->decl;
		struct emu_ev ev = {
			.payload = (union ovni_ev_payload *) &c->payload
		};
		if (ev_spec_compile(&spec, decl) != c->ret_compile)
			die("compile return mismatch for %s", decl->signature);

		/* Only print if compiled worked */
		if (c->ret_compile == 0) {
			if (ev_spec_print(&spec, &ev, buf, bufsize) != c->ret_print)
				die("print return mismatch for %s", decl->signature);

			/* Only check buffer if print worked */
			if (c->ret_print == 0) {
				if (strcmp(buf, c->output) != 0)
					die("different output: '%s' != '%s'",
							buf, c->output);
				else
					info("same output");
			}
		}

		info("case %d/%d OK", i, n);
	}
}


int main(void)
{
	test_format();

	return 0;
}
