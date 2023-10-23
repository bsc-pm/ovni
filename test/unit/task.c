/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "common.h"
#include "emu/task.h"
#include "unittest.h"

static void
test_parallel(void)
{
	struct task_info info;
	struct task_stack stack;
	struct task_stack stack2;

	memset(&info, 0, sizeof(info));
	memset(&stack, 0, sizeof(stack));
	memset(&stack2, 0, sizeof(stack2));

	uint32_t type_id = 123;
	uint32_t task_id = 456;

	OK(task_type_create(&info, type_id, "parallel_task"));
	OK(task_create(&info, type_id, task_id, TASK_FLAG_PARALLEL));

	struct task *task = task_find(info.tasks, task_id);

	if (task == NULL)
		err("task_find failed");

	/* Attempt to run body with zero id */
	ERR(task_execute(&stack, task, 0));

	OK(task_execute(&stack, task, 1));
	OK(task_execute(&stack2, task, 2));

	/* Attempt to run a body which is already running */
	ERR(task_execute(&stack2, task, 1));

	/* Finish a non existant body */
	ERR(task_end(&stack, task, 42));

	/* Attempt to pause top without pause flag */
	ERR(task_pause(&stack, task, 1));

	/* Attempt to resume while running */
	ERR(task_resume(&stack, task, 1));

	OK(task_end(&stack, task, 1));
	OK(task_end(&stack2, task, 2));

	/* Try to run one body again without resurrect flag */
	ERR(task_execute(&stack, task, 1));

	err("ok");
}

int main(void)
{
	test_parallel();

	return 0;
}
