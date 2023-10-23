/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef BODY_H
#define BODY_H

#include <stdint.h>
#include "common.h"

enum body_flags {
	/* Allow bodies to pause */
	BODY_FLAG_PAUSE          = (1 << 0),
	/* Allow bodies to run again after death */
	BODY_FLAG_RESURRECT      = (1 << 1),
	/* Allow nested tasks to begin running without previously pausing the
	 * current task. This is only added for compatibility with Nanos6. */
	BODY_FLAG_RELAX_NESTING  = (1 << 2),
};

enum body_state {
	BODY_ST_CREATED = 1,
	BODY_ST_RUNNING,
	BODY_ST_PAUSED,
	BODY_ST_DEAD,
	BODY_ST_MAX,
};

struct body;
struct task;

struct body_info {
	/* Hash map of task bodies for a task */
	struct body *bodies;
};

struct body_stack {
	struct body *top;
};

USE_RET struct body *body_find(struct body_info *info, uint32_t body_id);
USE_RET struct body *body_create(struct body_info *info, struct task *task, uint32_t body_id, int flags);

USE_RET int body_execute(struct body_stack *stack, struct body *body);
USE_RET int body_pause(struct body_stack *stack, struct body *body);
USE_RET int body_resume(struct body_stack *stack, struct body *body);
USE_RET int body_end(struct body_stack *stack, struct body *body);

USE_RET int body_can_resurrect(struct body *body);
USE_RET int body_can_pause(struct body *body);
USE_RET long body_get_iteration(struct body *body);
USE_RET struct body *body_get_running(struct body_stack *stack);
USE_RET struct body *body_get_top(struct body_stack *stack);
USE_RET struct task *body_get_task(struct body *body);
USE_RET enum body_state body_get_state(struct body *body);
USE_RET uint32_t body_get_id(struct body *body);
USE_RET const char *body_get_state_name(struct body *body);

#endif /* BODY_H */
