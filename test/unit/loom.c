/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu/loom.h"
#include <string.h>
#include "common.h"
#include "emu/cpu.h"
#include "emu/proc.h"
#include "unittest.h"

char testloom[] = "node1";
int testproc = 1;

static void
test_bad_name(struct loom *loom)
{
	ERR(loom_init_begin(loom, "loom/blah"));
	ERR(loom_init_begin(loom, "/loom.123"));
	ERR(loom_init_begin(loom, "./loom.123"));
	OK(loom_init_begin(loom, "loom.123"));
	OK(loom_init_begin(loom, "foo"));

	err("ok");
}

static void
test_hostname(struct loom *loom)
{
	OK(loom_init_begin(loom, "node1.blah"));

	if (strcmp(loom->hostname, "node1") != 0)
		die("wrong hostname: %s", loom->hostname);

	err("ok");
}

static void
test_negative_cpu(struct loom *loom)
{
	OK(loom_init_begin(loom, testloom));

	struct cpu cpu;
	cpu_init_begin(&cpu, -1, -1, 0);

	ERR(loom_add_cpu(loom, &cpu));

	err("ok");
}

static void
test_duplicate_cpus(struct loom *loom)
{
	struct cpu cpu;
	OK(loom_init_begin(loom, testloom));
	cpu_init_begin(&cpu, 123, 123, 0);
	OK(loom_add_cpu(loom, &cpu));
	ERR(loom_add_cpu(loom, &cpu));

	err("ok");
}

static void
test_duplicate_procs(struct loom *loom)
{
	struct proc proc;
	OK(loom_init_begin(loom, testloom));
	OK(proc_init_begin(&proc, testproc));
	OK(loom_add_proc(loom, &proc));
	ERR(loom_add_proc(loom, &proc));

	err("ok");
}

int main(void)
{
	struct loom loom;

	test_bad_name(&loom);
	test_hostname(&loom);
	test_negative_cpu(&loom);
	test_duplicate_cpus(&loom);
	test_duplicate_procs(&loom);

	return 0;
}
