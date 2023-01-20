#define _POSIX_C_SOURCE 200809L

#include "emu/clkoff.h"
#include "common.h"
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
		die("fmemopen failed\n");

	struct clkoff table;
	clkoff_init(&table);
	if (clkoff_load(&table, f) != 0)
		die("clkoff_load failed\n");

	if (clkoff_count(&table) != 4)
		die("clkoff_count failed\n");

	struct clkoff_entry *entry = clkoff_get(&table, 3);
	if (entry == NULL)
		die("clkoff_get returned NULL\n");

	if (entry->index != 3)
		die("clkoff_get returned wrong index\n");

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
		die("fmemopen failed\n");

	struct clkoff table;

	clkoff_init(&table);
	if (clkoff_load(&table, f) == 0)
		die("clkoff_load didn't fail\n");

	fclose(f);

	return 0;
}

int main(void)
{
	test_ok();
	test_dup();
	return 0;
}
