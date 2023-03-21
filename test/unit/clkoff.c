/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 200809L

#include "emu/clkoff.h"
#include "common.h"
#include "unittest.h"
#include <stdio.h>

static int
test_ok(void)
{
	char table_str[] =
	"rank       hostname             offset_median        offset_mean          offset_std\n"
	"0          s09r2b21             0                    0.000000             0.000000\n"
	"1          s09r2b22             -451607967           -451608083.500000    316.087397\n"
	"2          s09r2b23             4526                 4542.200000          124.33432\n"
	"3          s09r2b24             342455               342462.300000        342.39755\n";

	FILE *f = fmemopen(table_str, ARRAYLEN(table_str), "r");

	if (f == NULL)
		die("fmemopen failed:");

	struct clkoff table;
	clkoff_init(&table);
	OK(clkoff_load(&table, f));

	if (clkoff_count(&table) != 4)
		die("clkoff_count failed");

	struct clkoff_entry *entry = clkoff_get(&table, 3);
	if (entry == NULL)
		die("clkoff_get returned NULL");

	if (entry->index != 3)
		die("clkoff_get returned wrong index");

	fclose(f);

	return 0;
}

static int
test_dup(void)
{
	static char table_str[] =
	"rank       hostname             offset_median        offset_mean          offset_std\n"
	"0          s09r2b21             0                    0.000000             0.000000\n"
	"1          s09r2b22             -451607967           -451608083.500000    316.087397\n"
	"2          s09r2b23             4526                 4542.200000          124.33432\n"
	"3          s09r2b23             342455               342462.300000        342.39755\n";

	FILE *f = fmemopen(table_str, ARRAYLEN(table_str), "r");

	if (f == NULL)
		die("fmemopen failed:");

	struct clkoff table;

	clkoff_init(&table);
	ERR(clkoff_load(&table, f));

	fclose(f);

	return 0;
}

int main(void)
{
	test_ok();
	test_dup();
	return 0;
}
