/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include "common.h"
#include "pv/pcf.h"
#include "uthash.h"
#include "body.h"

enum task_flags {
	TASK_FLAG_PARALLEL      = (1 << 0), /* Can have multiple bodies */
	TASK_FLAG_RESURRECT     = (1 << 1), /* Can run again after dead */
	TASK_FLAG_PAUSE         = (1 << 2), /* Can pause the bodies */
	TASK_FLAG_RELAX_NESTING = (1 << 3), /* Can nest a over a running task */
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
	long nbodies;
	struct body_info body_info;
	uint32_t flags;

	/* Hash map in task_info */
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
	struct body_stack body_stack;
};

USE_RET uint32_t task_get_id(struct task *task);
USE_RET struct task *task_find(struct task *tasks, uint32_t task_id);
USE_RET int task_create(struct task_info *info, uint32_t type_id, uint32_t task_id, uint32_t flags);

USE_RET int task_execute(struct task_stack *stack, struct task *task, uint32_t body_id);
USE_RET int task_pause(struct task_stack *stack, struct task *task, uint32_t body_id);
USE_RET int task_resume(struct task_stack *stack, struct task *task, uint32_t body_id);
USE_RET int task_end(struct task_stack *stack, struct task *task, uint32_t body_id);

USE_RET struct task_type *task_type_find(struct task_type *types, uint32_t type_id);
USE_RET int task_type_create(struct task_info *info, uint32_t type_id, const char *label);
USE_RET uint32_t task_get_type_gid(const char *label);
USE_RET int task_create_pcf_types(struct pcf_type *pcftype, struct task_type *types);
USE_RET struct body *task_get_running(struct task_stack *stack);
USE_RET struct body *task_get_top(struct task_stack *stack);
USE_RET int task_is_parallel(struct task *task);

#endif /* TASK_H */
