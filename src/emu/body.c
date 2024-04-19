/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "body.h"
#include "task.h"
#include <string.h>
#include "uthash.h"
#include "utlist.h"

/* Make body private struct */
struct body {
	/* Body id consecutive and unique per task */
	uint32_t id;
	uint32_t taskid;
	int flags;
	enum body_state state;
	UT_hash_handle hh;
	struct body_stack *stack;
	long iteration;
	struct task *task;

	/* List handle for nested task support */
	struct body *next;
	struct body *prev;

	/* Create a identification name */
	char name[256];
};

struct body *
body_find(struct body_info *info, uint32_t body_id)
{
	struct body *body = NULL;
	HASH_FIND_INT(info->bodies, &body_id, body);

	return body;
}

struct body *
body_create(struct body_info *info, struct task *task, uint32_t body_id, int flags)
{
	if (body_id == 0) {
		err("body id %u must be non-zero", body_id);
		return NULL;
	}

	if (body_find(info, body_id) != NULL) {
		err("body with id %u already exists", body_id);
		return NULL;
	}

	if (task == NULL) {
		err("task of body %u is NULL", body_id);
		return NULL;
	}

	struct body *body = calloc(1, sizeof(struct body));

	if (body == NULL) {
		err("calloc failed:");
		return NULL;
	}

	body->id = body_id;
	body->state = BODY_ST_CREATED;
	body->iteration = 0;
	body->flags = flags;
	body->task = task;
	body->taskid = task_get_id(task); /* For debug purposes only */

	if (snprintf(body->name, sizeof(body->name), "body(id=%u,taskid=%u)",
			body->id, body->taskid) >= (int) sizeof(body->name)) {
		err("body name too long");
		return NULL;
	}

	/* Add the new body to the hash table */
	HASH_ADD_INT(info->bodies, id, body);

	return body;
}

/* Transition from Created to Running */
int
body_execute(struct body_stack *stack, struct body *body)
{
	if (body == NULL) {
		err("body is NULL");
		return -1;
	}

	/* Allow bodies to be executed multiple times */
	if (body->state == BODY_ST_DEAD) {
		if (!body_can_resurrect(body)) {
			err("%s is not allowed to run again", body->name);
			return -1;
		}
		body->state = BODY_ST_CREATED;
		body->iteration++;
		dbg("%s runs again", body->name);
	}

	/* Better error for executing a paused task body */
	if (body->state == BODY_ST_PAUSED) {
		err("refusing to run %s in Paused state, needs to resume intead",
				body->name);
		return -1;
	}

	if (body->state != BODY_ST_CREATED) {
		err("%s state must be Created but is %s",
				body->name, body_get_state_name(body));
		return -1;
	}

	if (body->stack != NULL) {
		err("%s stack already set", body->name);
		return -1;
	}

	struct body *top = body_get_running(stack);
	if (top != NULL) {
		if (top->flags & BODY_FLAG_RELAX_NESTING) {
			warn("deprecated nesting %s over already running %s",
					body->name, top->name);
		} else {
			err("cannot nest %s over %s which is already running",
					body->name, top->name);
			return -1;
		}
	}

	body->stack = stack;
	body->state = BODY_ST_RUNNING;

	DL_PREPEND(stack->top, body);

	dbg("%s state is now Running, iteration %lld",
			body->name, body->iteration);

	return 0;
}

/* Transition from Running to Paused */
int
body_pause(struct body_stack *stack, struct body *body)
{
	if (body == NULL) {
		err("body is NULL");
		return -1;
	}

	if (!body_can_pause(body)) {
		err("%s is not allowed to pause", body->name);
		return -1;
	}

	if (body->state != BODY_ST_RUNNING) {
		err("%s state must be Running but is %s",
				body->name, body_get_state_name(body));
		return -1;
	}

	if (body->stack == NULL) {
		err("%s stack not set", body->name);
		return -1;
	}

	if (body->stack != stack) {
		err("%s has another stack", body->name);
		return -1;
	}

	if (stack->top != body) {
		err("%s is not on top of the stack", body->name);
		return -1;
	}

	body->state = BODY_ST_PAUSED;

	dbg("%s state is now Paused", body->name);

	return 0;
}

/* Transition from Paused to Running */
int
body_resume(struct body_stack *stack, struct body *body)
{
	if (body == NULL) {
		err("body is NULL");
		return -1;
	}

	if (body->state != BODY_ST_PAUSED) {
		err("%s state must be Paused but is %s",
				body->name, body_get_state_name(body));
		return -1;
	}

	if (body->stack == NULL) {
		err("%s stack not set", body->name);
		return -1;
	}

	if (body->stack != stack) {
		err("%s has another stack", body->name);
		return -1;
	}

	if (stack->top != body) {
		err("%s is not on top of the stack", body->name);
		return -1;
	}

	body->state = BODY_ST_RUNNING;

	dbg("%s state is now Running", body->name);

	return 0;
}

/* Transition from Running to Dead */
int
body_end(struct body_stack *stack, struct body *body)
{
	if (body == NULL) {
		err("body is NULL");
		return -1;
	}

	if (body->state != BODY_ST_RUNNING) {
		err("%s state must be Running but is %s",
				body->name, body_get_state_name(body));
		return -1;
	}

	if (body->stack == NULL) {
		err("%s stack not set", body->name);
		return -1;
	}

	if (body->stack != stack) {
		err("%s has another stack", body->name);
		return -1;
	}

	if (stack->top != body) {
		err("%s is not on top of the stack", body->name);
		return -1;
	}

	body->state = BODY_ST_DEAD;

	/* FIXME: This should me changed:
	 * "Don't unset the thread from the task, as it will be used
	 * later to ensure we switch to tasks of the same thread." */
	DL_DELETE(stack->top, body);
	body->stack = NULL;

	dbg("%s state is now Dead, completed iteration %lld",
			body->name, body->iteration);

	return 0;
}

int
body_can_resurrect(struct body *body)
{
	return (body->flags & BODY_FLAG_RESURRECT);
}

int
body_can_pause(struct body *body)
{
	return (body->flags & BODY_FLAG_PAUSE);
}

/** Return the iteration number of the body.
 *
 * Starts at 0 for the first execution and increases every time it is executed
 * again */
long
body_get_iteration(struct body *body)
{
	return body->iteration;
}

/** Returns the top body in the stack if is running, otherwise NULL. */
struct body *
body_get_running(struct body_stack *stack)
{
	struct body *body = stack->top;
	if (body && body->state == BODY_ST_RUNNING)
		return body;

	return NULL;
}

/** Returns the top body in the stack. */
struct body *
body_get_top(struct body_stack *stack)
{
	return stack->top;
}

uint32_t
body_get_id(struct body *body)
{
	return body->id;
}

struct task *
body_get_task(struct body *body)
{
	return body->task;
}

enum body_state
body_get_state(struct body *body)
{
	return body->state;
}

const char *
body_get_state_name(struct body *body)
{
	const char *name[BODY_ST_MAX] = {
		[BODY_ST_CREATED] = "Created",
		[BODY_ST_RUNNING] = "Running",
		[BODY_ST_PAUSED]  = "Paused",
		[BODY_ST_DEAD]    = "Dead",
	};

	return name[body->state];
}
