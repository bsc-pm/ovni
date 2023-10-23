/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "common.h"
#include "emu/task.h"
#include "emu/body.h"
#include "unittest.h"
#include <string.h>

static void
test_body(void)
{
	struct task task = { .id = 123 };
	struct body_info info;
	struct body_stack stack1;

	memset(&info, 0, sizeof(info));
	memset(&stack1, 0, sizeof(stack1));

	/* Create 10 bodies: no pause and no resurrect */
	for (uint32_t id = 1; id <= 10; id++) {
		if (body_create(&info, &task, id, 0) == NULL)
			die("body_create failed");
	}

	/* Try to create another body with the same id */
	if (body_create(&info, &task, 3, 0) != NULL)
		die("body_create didn't fail");

	/* Try to create a body with id 0 */
	if (body_create(&info, &task, 0, 0) != NULL)
		die("body_create didn't fail");

	/* Run a body */
	struct body *body1 = body_find(&info, 1);
	if (body1 == NULL)
		die("body_find failed");
	OK(body_execute(&stack1, body1));

	/* Finish a non existant body */
	ERR(body_end(&stack1, NULL));

	/* Attempt to run a body on top of one that is already running */
	struct body_stack stack2;
	memset(&stack2, 0, sizeof(stack2));
	struct body *body2 = body_find(&info, 2);
	if (body2 == NULL)
		die("body_find failed");
	OK(body_execute(&stack2, body2));
	ERR(body_execute(&stack1, body2));

	/* Attempt to stop a body that is not the top */
	ERR(body_end(&stack1, body2));

	/* Attempt to pause the top body without the pause flag */
	ERR(body_pause(&stack1, body1));

	/* Attempt ending a task from another stack */
	ERR(body_end(&stack2, body1));

	/* Stop the bodies */
	OK(body_end(&stack1, body1));
	OK(body_end(&stack2, body2));

	/* Try to run an already created body again without the resurrect flag */
	ERR(body_execute(&stack1, body1));

	err("ok");
}

static void
test_pause(void)
{
	struct task task = { .id = 123 };
	struct body *body;
	struct body_info info;
	struct body_stack stack;
	struct body_stack stack2;

	memset(&info, 0, sizeof(info));
	memset(&stack, 0, sizeof(stack));
	memset(&stack2, 0, sizeof(stack2));

	/* Create 10 bodies: only pause */
	for (uint32_t id = 1; id <= 10; id++) {
		if (body_create(&info, &task, id, BODY_FLAG_PAUSE) == NULL)
			die("body_create failed");
	}

	/* Run the 10 bodies one on top of the other */
	for (uint32_t id = 1; id <= 10; id++) {
		body = body_find(&info, id);
		if (body == NULL)
			die("body_find failed");
		OK(body_execute(&stack, body));

		/* Don't pause the last one */
		if (id != 10)
			OK(body_pause(&stack, body));
	}

	/* Attempt to run a body which is already running */
	body = body_find(&info, 3);
	if (body == NULL)
		die("body_find failed");
	ERR(body_execute(&stack, body));

	/* Attempt to end a body that is not the top */
	ERR(body_end(&stack, body));

	/* Pause the top */
	body = body_get_top(&stack);
	OK(body_pause(&stack, body));

	/* Attempt to resume in another stack */
	ERR(body_resume(&stack2, body));

	/* Stop the bodies */
	for (uint32_t id = 10; id >= 1; id--) {
		body = body_find(&info, id);
		if (body == NULL)
			die("body_find failed");
		OK(body_resume(&stack, body));
		OK(body_end(&stack, body));
	}

	err("ok");
}

static void
test_resurrect(void)
{
	struct task task = { .id = 123 };
	struct body *body;
	struct body_info info;
	struct body_stack stack;

	memset(&info, 0, sizeof(info));
	memset(&stack, 0, sizeof(stack));

	/* Create a body with id 1 that can resurrect */
	if ((body = body_create(&info, &task, 1, BODY_FLAG_RESURRECT)) == NULL)
		die("body_create failed");

	/* Run it and end it */
	OK(body_execute(&stack, body));
	OK(body_end(&stack, body));

	/* Then try to run it again */
	OK(body_execute(&stack, body));
	OK(body_end(&stack, body));

	err("ok");
}

int main(void)
{
	test_body();
	test_pause();
	test_resurrect();

	return 0;
}
