/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu/cpu.h"
#include "common.h"
#include "emu/loom.h"
#include "emu/proc.h"
#include "emu/thread.h"
#include "unittest.h"
#include "parson.h"

static void
test_oversubscription(void)
{
	struct loom loom;
	OK(loom_init_begin(&loom, "loom.0"));

	struct cpu cpu;

	int phyid = 0;
	int index = 0;
	cpu_init_begin(&cpu, index, phyid, 0);
	cpu_set_gindex(&cpu, 0);
	cpu_set_loom(&cpu, &loom);
	if (cpu_init_end(&cpu) != 0)
		die("cpu_init_end failed");

	struct proc proc;

	if (proc_init_begin(&proc, 1) != 0)
		die("proc_init_begin failed");

	proc.appid = 1;

	proc_set_gindex(&proc, 0);

	struct thread th0, th1;

	if (thread_init_begin(&th0, 1) != 0)
		die("thread_init_begin failed");

	thread_set_gindex(&th0, 0);
	th0.meta = (JSON_Object *) 666;

	if (thread_init_end(&th0) != 0)
		die("thread_init_end failed");

	if (thread_init_begin(&th1, 2) != 0)
		die("thread_init_begin failed");

	thread_set_gindex(&th1, 1);
	th1.meta = (JSON_Object *) 666;

	if (thread_init_end(&th1) != 0)
		die("thread_init_end failed");

	if (proc_add_thread(&proc, &th0) != 0)
		die("proc_add_thread failed");

	if (proc_add_thread(&proc, &th1) != 0)
		die("proc_add_thread failed");

	if (proc_init_end(&proc) != 0)
		die("proc_init_end failed");

	if (thread_set_cpu(&th0, &cpu) != 0)
		die("thread_set_cpu failed");

	if (thread_set_cpu(&th1, &cpu) != 0)
		die("thread_set_cpu failed");

	if (thread_set_state(&th0, TH_ST_RUNNING) != 0)
		die("thread_set_state failed");

	if (thread_set_state(&th1, TH_ST_RUNNING) != 0)
		die("thread_set_state failed");

	if (cpu_add_thread(&cpu, &th0) != 0)
		die("cpu_add_thread failed");

	if (cpu_add_thread(&cpu, &th1) == 0)
		die("cpu_add_thread didn't fail");

	err("ok");
}

int main(void)
{
	test_oversubscription();

	return 0;
}
