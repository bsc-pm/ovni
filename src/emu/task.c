/* Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "task.h"

#include "utlist.h"

struct task *
task_find(struct task *tasks, uint32_t task_id)
{
	struct task *task = NULL;
	HASH_FIND_INT(tasks, &task_id, task);

	return task;
}

struct task_type *
task_type_find(struct task_type *types, uint32_t type_id)
{
	struct task_type *type = NULL;
	HASH_FIND_INT(types, &type_id, type);

	return type;
}

int
task_create(struct task_info *info, uint32_t type_id, uint32_t task_id)
{
	/* Ensure the task id is new */
	if (task_find(info->tasks, task_id) != NULL) {
		err("task_id %u already exists", task_id);
		return -1;
	}

	/* Ensure the type exists */
	struct task_type *type = task_type_find(info->types, type_id);
	if (type == NULL) {
		err("unknown type id %u", type_id);
		return -1;
	}

	struct task *task = calloc(1, sizeof(struct task));

	if (task == NULL) {
		err("calloc failed:");
		return -1;
	}

	task->id = task_id;
	task->type = type;
	task->state = TASK_ST_CREATED;
	task->thread = NULL;

	/* Add the new task to the hash table */
	HASH_ADD_INT(info->tasks, id, task);

	dbg("new task created id=%d", task->id);
	return 0;
}

int
task_execute(struct task_stack *stack, struct task *task)
{
	if (task == NULL) {
		err("task is NULL");
		return -1;
	}

	if (task->state != TASK_ST_CREATED) {
		err("cannot execute task %u: state is not created", task->id);
		return -1;
	}

	if (task->thread != NULL) {
		err("task already has a thread assigned");
		return -1;
	}

	if (stack->thread->state != TH_ST_RUNNING) {
		err("thread state is not running");
		return -1;
	}

	if (stack->top == task) {
		err("thread already has assigned task %u", task->id);
		return -1;
	}

	if (stack->top && stack->top->state != TASK_ST_RUNNING) {
		err("cannot execute a nested task from a non-running task");
		return -1;
	}

	task->state = TASK_ST_RUNNING;
	task->thread = stack->thread;

	DL_PREPEND(stack->tasks, task);

	dbg("task id=%u runs now", task->id);
	return 0;
}

int
task_pause(struct task_stack *stack, struct task *task)
{
	if (task == NULL) {
		err("cannot pause: task is NULL");
		return -1;
	}

	if (task->state != TASK_ST_RUNNING) {
		err("cannot pause: task state is not running");
		return -1;
	}

	if (task->thread == NULL) {
		err("cannot pause: task has no thread assigned");
		return -1;
	}

	if (stack->thread->state != TH_ST_RUNNING) {
		err("cannot pause: thread state is not running");
		return -1;
	}

	if (stack->top != task) {
		err("thread has assigned a different task");
		return -1;
	}

	if (stack->thread != task->thread) {
		err("task is assigned to a different thread");
		return -1;
	}

	task->state = TASK_ST_PAUSED;

	dbg("task id=%d pauses", task->id);
	return 0;
}

int
task_resume(struct task_stack *stack, struct task *task)
{
	if (task == NULL) {
		err("cannot resume: task is NULL");
		return -1;
	}

	if (task->state != TASK_ST_PAUSED) {
		err("task state is not paused");
		return -1;
	}

	if (task->thread == NULL) {
		err("cannot resume: task has no thread assigned");
		return -1;
	}

	if (stack->thread->state != TH_ST_RUNNING) {
		err("thread is not running");
		return -1;
	}

	if (stack->top != task) {
		err("thread has assigned a different task");
		return -1;
	}

	if (stack->thread != task->thread) {
		err("task is assigned to a different thread");
		return -1;
	}

	task->state = TASK_ST_RUNNING;

	dbg("task id=%d resumes", task->id);
	return 0;
}

int
task_end(struct task_stack *stack, struct task *task)
{
	if (task == NULL) {
		err("cannot end: task is NULL");
		return -1;
	}

	if (task->state != TASK_ST_RUNNING) {
		err("task state is not running");
		return -1;
	}

	if (task->thread == NULL) {
		err("cannot end: task has no thread assigned");
		return -1;
	}

	if (stack->thread->state != TH_ST_RUNNING) {
		err("cannot end task: thread is not running");
		return -1;
	}

	if (stack->top != task) {
		err("thread has assigned a different task");
		return -1;
	}

	if (stack->thread != task->thread) {
		err("task is assigned to a different thread");
		return -1;
	}

	task->state = TASK_ST_DEAD;

	/* Don't unset the thread from the task, as it will be used
	 * later to ensure we switch to tasks of the same thread. */

	DL_DELETE(stack->tasks, task);

	dbg("task id=%d ends", task->id);
	return 0;
}

static uint32_t
get_task_type_gid(const char *label)
{
	uint32_t gid;

	HASH_VALUE(label, strlen(label), gid);

	/* Use non-negative values */
	gid &= 0x7FFFFFFF;

	/* Avoid bad colors for "Unlabeled0" and "main" */
	gid += 666;

	if (gid == 0)
		gid++;

	return gid;
}

int
task_type_create(struct task_info *info, uint32_t type_id, const char *label)
{
	struct task_type *type;
	/* Ensure the type id is new */
	HASH_FIND_INT(info->types, &type_id, type);

	if (type != NULL) {
		err("a task type with id %u already exists", type_id);
		return -1;
	}

	type = calloc(1, sizeof(*type));
	if (type == NULL) {
		err("calloc failed:");
		return -1;
	}

	type->id = type_id;

	if (type->id == 0) {
		err("invalid task type id %d", type->id);
		return -1;
	}

	type->gid = get_task_type_gid(label);
	int n = snprintf(type->label, MAX_PCF_LABEL, "%s", label);
	if (n >= MAX_PCF_LABEL) {
		err("task type label too long: %s", label);
		return -1;
	}

	/* Add the new task type to the hash table */
	HASH_ADD_INT(info->types, id, type);

	dbg("new task type created id=%d label=%s", type->id, type->label);
	return 0;
}

int
task_create_pcf_types(struct pcf_type *pcftype, struct task_type *types)
{
	int ret = 0;

	/* Emit types for all task types */
	for (struct task_type *tt = types; tt != NULL; tt = tt->hh.next) {
		struct pcf_value *pcfvalue = pcf_find_value(pcftype, tt->gid);
		if (pcfvalue != NULL) {
			/* Ensure the label is the same, so we know that
			 * no collision occurred */
			if (strcmp(pcfvalue->label, tt->label) != 0) {
				err("collision occurred in task type labels: %s and %s",
						pcfvalue->label, tt->label);
				ret = -1;
			} else {
				continue;
			}
		}

		pcf_add_value(pcftype, tt->gid, tt->label);
	}

	return ret;
}

struct task *
task_get_running(struct task_stack *stack)
{
	struct task *task = stack->top;
	if (task && task->state == TASK_ST_RUNNING)
		return task;

	return NULL;
}
