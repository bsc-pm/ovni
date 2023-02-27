/* Copyright (c) 2022-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "uthash.h"
#include "pv/pcf.h"
#include "thread.h"

enum task_state {
	TASK_ST_CREATED,
	TASK_ST_RUNNING,
	TASK_ST_PAUSED,
	TASK_ST_DEAD,
};

struct task_type {
	uint32_t id;  /* Per-process task identifier */
	uint32_t gid; /* Global identifier computed from the label */
	char label[MAX_PCF_LABEL];
	UT_hash_handle hh;
};

struct task {
	uint32_t id;
	struct task_type *type;

	/* TODO: Use a pointer to task_stack instead of thread */
	/* The thread that has began to execute the task. It cannot
	 * changed after being set, even if the task ends. */
	struct thread *thread;
	enum task_state state;
	UT_hash_handle hh;

	/* List handle for nested task support */
	struct task *next;
	struct task *prev;
};

struct task_info {
	/* Both hash maps of all known tasks and types */
	struct task_type *types;
	struct task *tasks;
};

struct task_stack {
	union {
		struct task *top; /* Synctactic sugar */
		struct task *tasks;
	};
	struct thread *thread;
};

USE_RET struct task *task_find(struct task *tasks, uint32_t task_id);
USE_RET int task_create(struct task_info *info, uint32_t type_id, uint32_t task_id);
USE_RET int task_execute(struct task_stack *stack, struct task *task);
USE_RET int task_pause(struct task_stack *stack, struct task *task);
USE_RET int task_resume(struct task_stack *stack, struct task *task);
USE_RET int task_end(struct task_stack *stack, struct task *task);
USE_RET struct task_type *task_type_find(struct task_type *types, uint32_t type_id);
USE_RET int task_type_create(struct task_info *info, uint32_t type_id, const char *label);
USE_RET uint32_t task_get_type_gid(const char *label);
USE_RET int task_create_pcf_types(struct pcf_type *pcftype, struct task_type *types);
USE_RET struct task *task_get_running(struct task_stack *stack);

#endif /* TASK_H */
