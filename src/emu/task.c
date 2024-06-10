/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thread.h"
#include "pv/pcf.h"
#include "utlist.h"

uint32_t
task_get_id(struct task *task)
{
	return task->id;
}

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
task_create(struct task_info *info, uint32_t type_id, uint32_t task_id, uint32_t flags)
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
	task->flags = flags;

	/* Add the new task to the hash table */
	HASH_ADD_INT(info->tasks, id, task);

	dbg("new task created id=%d", task->id);
	return 0;
}

static struct body *
create_body(struct task *task, uint32_t body_id)
{
	if (!task_is_parallel(task) && task->nbodies > 0) {
		err("cannot create more than one body for non-parallel task %u",
				task->id);
		return NULL;
	}

	int flags = 0;

	if (task->flags & TASK_FLAG_RELAX_NESTING)
		flags |= BODY_FLAG_RELAX_NESTING;

	if (task->flags & TASK_FLAG_RESURRECT)
		flags |= BODY_FLAG_RESURRECT;

	if (task->flags & TASK_FLAG_PAUSE)
		flags |= BODY_FLAG_PAUSE;

	struct body *body = body_create(&task->body_info, task, body_id, flags);

	if (body == NULL) {
		err("body_create failed for task %u", task->id);
		return NULL;
	}

	task->nbodies++;

	return body;
}

int
task_execute(struct task_stack *stack, struct task *task, uint32_t body_id)
{
	if (task == NULL) {
		err("task is NULL");
		return -1;
	}

	struct body *body = body_find(&task->body_info, body_id);

	/* Create new body if it doesn't exist. */
	if (body == NULL) {
		body = create_body(task, body_id);

		if (body == NULL) {
			err("create_body failed");
			return -1;
		}
	}

	if (body_execute(&stack->body_stack, body) != 0) {
		err("body_execute failed for task %u", task->id);
		return -1;
	}

	dbg("body %u of task %u executes", body_id, task->id);

	return 0;
}

int
task_pause(struct task_stack *stack, struct task *task, uint32_t body_id)
{
	if (task == NULL) {
		err("task is NULL");
		return -1;
	}

	struct body *body = body_find(&task->body_info, body_id);

	if (body == NULL) {
		err("cannot find body with id %u in task %u",
				body_id, task->id);
		return -1;
	}

	if (body_pause(&stack->body_stack, body) != 0) {
		err("body_pause failed for task %u", task->id);
		return -1;
	}

	dbg("body %u of task %u pauses", body_id, task->id);

	return 0;
}

int
task_resume(struct task_stack *stack, struct task *task, uint32_t body_id)
{
	if (task == NULL) {
		err("task is NULL");
		return -1;
	}

	struct body *body = body_find(&task->body_info, body_id);

	if (body == NULL) {
		err("cannot find body with id %u in task %u",
				body_id, task->id);
		return -1;
	}

	if (body_resume(&stack->body_stack, body) != 0) {
		err("body_resume failed for task %u", task->id);
		return -1;
	}

	dbg("body %u of task %u resumes", body_id, task->id);

	return 0;
}

int
task_end(struct task_stack *stack, struct task *task, uint32_t body_id)
{
	if (task == NULL) {
		err("task is NULL");
		return -1;
	}

	struct body *body = body_find(&task->body_info, body_id);

	if (body == NULL) {
		err("cannot find body with id %u in task %u",
				body_id, task->id);
		return -1;
	}

	if (body_end(&stack->body_stack, body) != 0) {
		err("body_end failed for task %u", task->id);
		return -1;
	}

	dbg("body %u of task %u ends", body_id, task->id);

	return 0;
}

int
task_is_parallel(struct task *task)
{
	return (task->flags & TASK_FLAG_PARALLEL);
}

uint32_t
task_get_type_gid(const char *label)
{
	uint32_t gid;

	HASH_VALUE(label, strlen(label), gid);

	/* Avoid bad colors for "Unlabeled0" and "main" */
	gid += 666;

	/* Use non-negative values */
	gid &= 0x7FFFFFFF;

	/* Avoid reserved values */
	if (gid < PCF_RESERVED)
		gid += PCF_RESERVED;

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

	int n;
	if (label[0] == '\0') {
		/* Give a name if empty */
		n = snprintf(type->label, MAX_PCF_LABEL,
				"(unlabeled task type %u)", type_id);
	} else {
		n = snprintf(type->label, MAX_PCF_LABEL, "%s", label);
	}

	if (n >= MAX_PCF_LABEL) {
		err("task type %u label too long", type_id);
		return -1;
	}

	type->gid = task_get_type_gid(type->label);

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

		if (pcf_add_value(pcftype, tt->gid, tt->label) == NULL) {
			err("pcf_add_value failed");
			return -1;
		}
	}

	return ret;
}

struct body *
task_get_running(struct task_stack *stack)
{
	return body_get_running(&stack->body_stack);
}

struct body *
task_get_top(struct task_stack *stack)
{
	return body_get_top(&stack->body_stack);
}
