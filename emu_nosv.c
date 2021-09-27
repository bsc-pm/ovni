#include <assert.h>
#include "uthash.h"

#include "ovni.h"
#include "ovni_trace.h"
#include "emu.h"
#include "prv.h"

enum nosv_prv_type {
	PRV_TYPE_PROCID
};

struct hook_entry {
	char id[4];
	void (*hook)(struct ovni_emu *);
};

/* --------------------------- pre ------------------------------- */

static void
pre_task_create(struct ovni_emu *emu)
{
	struct nosv_task *task, *p = NULL;
	
	task = calloc(1, sizeof(*task));

	if(task == NULL)
	{
		perror("calloc");
		abort();
	}

	task->id = emu->cur_ev->payload.i32[0];
	task->type_id = emu->cur_ev->payload.i32[1];
	task->state = TASK_ST_CREATED;

	/* Ensure the task id is new */
	HASH_FIND_INT(emu->cur_proc->tasks, &task->id, p);

	if(p != NULL)
	{
		err("A task with id %d already exists\n", p->id);
		abort();
	}

	/* Add the new task to the hash table */
	HASH_ADD_INT(emu->cur_proc->tasks, id, task);

	emu->cur_task = task;

	dbg("new task created id=%d\n", task->id);
}

static void
pre_task_execute(struct ovni_emu *emu)
{
	struct nosv_task *task;
	int taskid;

	taskid = emu->cur_ev->payload.i32[0];

	HASH_FIND_INT(emu->cur_proc->tasks, &taskid, task);

	assert(task != NULL);
	assert(emu->cur_thread->state == TH_ST_RUNNING);
	assert(emu->cur_thread->task == NULL);

	task->state = TASK_ST_RUNNING;
	task->thread = emu->cur_thread;
	emu->cur_thread->task = task;

	emu->cur_task = task;

	dbg("task id=%d runs now\n", task->id);
}

static void
pre_task_pause(struct ovni_emu *emu)
{
	struct nosv_task *task;
	int taskid;

	taskid = emu->cur_ev->payload.i32[0];

	HASH_FIND_INT(emu->cur_proc->tasks, &taskid, task);

	assert(task != NULL);
	assert(task->state == TASK_ST_RUNNING);
	assert(emu->cur_thread->state == TH_ST_RUNNING);
	assert(emu->cur_thread->task == task);
	assert(emu->cur_thread == task->thread);

	task->state = TASK_ST_PAUSED;

	emu->cur_task = task;

	dbg("task id=%d pauses\n", task->id);
}

static void
pre_task_resume(struct ovni_emu *emu)
{
	struct nosv_task *task;
	int taskid;

	taskid = emu->cur_ev->payload.i32[0];

	HASH_FIND_INT(emu->cur_proc->tasks, &taskid, task);

	assert(task != NULL);
	assert(task->state == TASK_ST_PAUSED);
	assert(emu->cur_thread->state == TH_ST_RUNNING);
	assert(emu->cur_thread->task == task);
	assert(emu->cur_thread == task->thread);

	task->state = TASK_ST_RUNNING;

	emu->cur_task = task;

	dbg("task id=%d resumes\n", task->id);
}


static void
pre_task_end(struct ovni_emu *emu)
{
	struct nosv_task *task;
	int taskid;

	taskid = emu->cur_ev->payload.i32[0];

	HASH_FIND_INT(emu->cur_proc->tasks, &taskid, task);

	assert(task != NULL);
	assert(task->state == TASK_ST_RUNNING);
	assert(emu->cur_thread->state == TH_ST_RUNNING);
	assert(emu->cur_thread->task == task);
	assert(emu->cur_thread == task->thread);

	task->state = TASK_ST_DEAD;
	task->thread = NULL;
	emu->cur_thread->task = NULL;

	emu->cur_task = task;

	dbg("task id=%d ends\n", task->id);
}

static void
pre_task(struct ovni_emu *emu)
{
	emu_emit(emu);
	switch(emu->cur_ev->header.value)
	{
		case 'c': pre_task_create(emu); break;
		case 'x': pre_task_execute(emu); break;
		case 'e': pre_task_end(emu); break;
		case 'p': pre_task_pause(emu); break;
		case 'r': pre_task_resume(emu); break;
		default:
			  emu->cur_task = NULL;
			  break;
	}
}

static void
pre_type_create(struct ovni_emu *emu)
{
	struct nosv_task_type *type;
	uint8_t *data;
	uint32_t *typeid;
	const char *label;

	if((emu->cur_ev->header.flags & OVNI_EV_JUMBO) == 0)
	{
		err("expecting a jumbo event\n");
		abort();
	}

	data = &emu->cur_ev->payload.jumbo.data[0];
	typeid = (uint32_t *) data;
	data += sizeof(*typeid);
	label = (const char *) data;

	/* Ensure the type id is new */
	HASH_FIND_INT(emu->cur_proc->types, typeid, type);

	if(type != NULL)
	{
		err("A task type with id %d already exists\n", *typeid);
		abort();
	}
	
	type = calloc(1, sizeof(*type));

	if(type == NULL)
	{
		perror("calloc");
		abort();
	}

	type->id = *typeid;
	type->label = label;

	/* Add the new task type to the hash table */
	HASH_ADD_INT(emu->cur_proc->types, id, type);

	dbg("new task type created id=%d label=%s\n", type->id,
			type->label);
}

static void
pre_type(struct ovni_emu *emu)
{
	switch(emu->cur_ev->header.value)
	{
		case 'c': pre_type_create(emu); break;
		default:
			  break;
	}
}

void
hook_pre_nosv(struct ovni_emu *emu)
{
	switch(emu->cur_ev->header.model)
	{
		/* Only listen for nosv events */
		case 'V':
			switch(emu->cur_ev->header.class)
			{
				case 'T': pre_task(emu); break;
				case 'Y': pre_type(emu); break;
				default:
					break;
			}
			break;
		default:
			break;
	}

	hook_pre_nosv_ss(emu);
}

/* --------------------------- emit ------------------------------- */

static void
emit_task_running(struct ovni_emu *emu, struct nosv_task *task)
{
	prv_ev_autocpu(emu, PTC_TASK_ID, task->id + 1);
	prv_ev_autocpu(emu, PTC_TASK_TYPE_ID, task->type_id + 1);
	prv_ev_autocpu(emu, PTC_APP_ID, emu->cur_proc->appid + 1);
}

static void
emit_task_not_running(struct ovni_emu *emu, struct nosv_task *task)
{
	prv_ev_autocpu(emu, PTC_TASK_ID, 0);
	prv_ev_autocpu(emu, PTC_TASK_TYPE_ID, 0);
	prv_ev_autocpu(emu, PTC_APP_ID, 0);
}

static void
emit_task(struct ovni_emu *emu)
{
	struct nosv_task *task;

	task = emu->cur_task;

	switch(emu->cur_ev->header.value)
	{
		case 'x':
		case 'r':
			emit_task_running(emu, task);
			break;
		case 'p':
		case 'e':
			emit_task_not_running(emu, task);
			break;
		case 'c':
		default:
			break;
	}
}

void
hook_emit_nosv(struct ovni_emu *emu)
{
	switch(emu->cur_ev->header.class)
	{
		case 'T': emit_task(emu); break;
		default:
			break;
	}

	hook_emit_nosv_ss(emu);
}

void
hook_post_nosv(struct ovni_emu *emu)
{
	hook_post_nosv_ss(emu);
}
