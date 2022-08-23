/*
 * Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "uthash.h"
#include "utlist.h"

#include "ovni.h"
#include "emu.h"
#include "emu_task.h"
#include "prv.h"
#include "chan.h"

void
task_create(uint32_t task_id, uint32_t type_id, struct task **task_map, struct task_type **type_map)
{
	/* Ensure the task id is new */
	struct task *task = NULL;
	HASH_FIND_INT(*task_map, &task_id, task);

	if(task != NULL)
		die("cannot create a task with id %u: already exists\n", task_id);

	/* Ensure the type exists */
	struct task_type *type = NULL;
	HASH_FIND_INT(*type_map, &type_id, type);

	if(type == NULL)
		die("cannot create task: unknown type id %u\n", type_id);

	task = calloc(1, sizeof(*task));

	if(task == NULL)
		die("calloc failed\n");

	task->id = task_id;
	task->type = type;
	task->state = TASK_ST_CREATED;
	task->thread = NULL;

	/* Add the new task to the hash table */
	HASH_ADD_INT(*task_map, id, task);

	dbg("new task created id=%d\n", task->id);
}

void
task_execute(uint32_t task_id, struct ovni_ethread *cur_thread, struct task **task_map, struct task **thread_task_stack)
{
	struct task *top = *thread_task_stack;
	struct task *task = NULL;
	HASH_FIND_INT(*task_map, &task_id, task);

	if(task == NULL)
		die("cannot find task with id %u\n", task_id);

	if(task->state != TASK_ST_CREATED)
		die("cannot execute task %u: state is not created\n", task_id);

	if(task->thread != NULL)
		die("task already has a thread assigned\n");

	if(cur_thread->state != TH_ST_RUNNING)
		die("thread state is not running\n");

	if(top == task)
		die("thread already has assigned task %u\n", task_id);

	if(top && top->state != TASK_ST_RUNNING)
		die("cannot execute a nested task from a non-running task\n");

	task->state = TASK_ST_RUNNING;
	task->thread = cur_thread;

	DL_PREPEND(*thread_task_stack, task);

	dbg("task id=%u runs now\n", task->id);
}

void
task_pause(uint32_t task_id, struct ovni_ethread *cur_thread, struct task **task_map, struct task **thread_task_stack)
{
	struct task *top = *thread_task_stack;
	struct task *task = NULL;
	HASH_FIND_INT(*task_map, &task_id, task);

	if(task == NULL)
		die("cannot find task with id %u\n", task_id);

	if(task->state != TASK_ST_RUNNING)
		die("task state is not running\n");

	if(cur_thread->state != TH_ST_RUNNING)
		die("thread state is not running\n");

	if(top != task)
		die("thread has assigned a different task\n");

	if(cur_thread != task->thread)
		die("task is assigned to a different thread\n");

	task->state = TASK_ST_PAUSED;

	dbg("task id=%d pauses\n", task->id);
}

void
task_resume(uint32_t task_id, struct ovni_ethread *cur_thread, struct task **task_map, struct task **thread_task_stack)
{
	struct task *top = *thread_task_stack;
	struct task *task = NULL;
	HASH_FIND_INT(*task_map, &task_id, task);

	if(task == NULL)
		die("cannot find task with id %u\n", task_id);

	if(task->state != TASK_ST_PAUSED)
		die("task state is not paused\n");

	if(cur_thread->state != TH_ST_RUNNING)
		die("thread is not running\n");

	if(top != task)
		die("thread has assigned a different task\n");

	if(cur_thread != task->thread)
		die("task is assigned to a different thread\n");

	task->state = TASK_ST_RUNNING;

	dbg("task id=%d resumes\n", task->id);
}

void
task_end(uint32_t task_id, struct ovni_ethread *cur_thread, struct task **task_map, struct task **thread_task_stack)
{
	struct task *top = *thread_task_stack;
	struct task *task = NULL;
	HASH_FIND_INT(*task_map, &task_id, task);

	if(task == NULL)
		die("cannot find task with id %u\n", task_id);

	if(task->state != TASK_ST_RUNNING)
		die("task state is not running\n");

	if(cur_thread->state != TH_ST_RUNNING)
		die("thread is not running\n");

	if(top != task)
		die("thread has assigned a different task\n");

	if(cur_thread != task->thread)
		die("task is assigned to a different thread\n");

	task->state = TASK_ST_DEAD;

	/* Don't unset the thread from the task, as it will be used
	 * later to ensure we switch to tasks of the same thread. */

	DL_DELETE(*thread_task_stack, task);

	dbg("task id=%d ends\n", task->id);
}

static uint32_t
get_task_type_gid(const char *label)
{
	uint32_t gid;

	HASH_VALUE(label, strlen(label), gid);

	/* Use non-negative values */
	gid &= 0x7FFFFFFF;

	if(gid == 0)
		gid++;

	return gid;
}

void
task_type_create(uint32_t typeid, const char *label, struct task_type **type_map)
{
	struct task_type *type;
	/* Ensure the type id is new */
	HASH_FIND_INT(*type_map, &typeid, type);

	if(type != NULL)
	{
		err("A task type with id %d already exists\n", typeid);
		abort();
	}

	type = calloc(1, sizeof(*type));

	if(type == NULL)
	{
		perror("calloc");
		abort();
	}

	type->id = typeid;

	if(type->id == 0)
		die("invalid task type id %d\n", type->id);

	type->gid = get_task_type_gid(label);
	int n = snprintf(type->label, MAX_PCF_LABEL, "%s", label);
	if(n >= MAX_PCF_LABEL)
		die("task label too long: %s\n", label);

	/* Add the new task type to the hash table */
	HASH_ADD_INT(*type_map, id, type);

	dbg("new task type created id=%d label=%s\n", type->id,
			type->label);
}

void
task_create_pcf_types(struct pcf_type *pcftype, struct task_type *type_map)
{
	/* Emit types for all task types */
	struct task_type *tt;
	for(tt = type_map; tt != NULL; tt = tt->hh.next)
	{
		struct pcf_value *pcfvalue = pcf_find_value(pcftype, tt->gid);
		if(pcfvalue != NULL)
		{
			/* Ensure the label is the same, so we know that
			 * no collision occurred */
			if(strcmp(pcfvalue->label, tt->label) != 0)
				die("collision occurred in task type labels\n");
			else
				continue;
		}

		pcf_add_value(pcftype, tt->gid, tt->label);
	}
}

struct task *
task_get_running(struct task *task_stack)
{
	struct task *task = task_stack;
	if(task && task->state == TASK_ST_RUNNING)
		return task;

	return NULL;
}
