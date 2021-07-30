#include "ovni.h"
#include "ovni_trace.h"
#include "emu.h"
#include "uthash.h"
#include <assert.h>

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
	dbg("pre nosv\n");
	switch(emu->cur_ev->header.class)
	{
		case 'T': pre_task(emu); break;
		case 'Y': pre_type(emu); break;
		default:
			  break;
	}
}

/* --------------------------- emit ------------------------------- */

static void
emit_task_create(struct ovni_emu *emu)
{
	//emu_emit_prv(emu, 200, emu->cur_task->id);
}

static void
emit_task_execute(struct ovni_emu *emu)
{
	emu_emit_prv(emu, 200, emu->cur_task->id + 1);
	emu_emit_prv(emu, 300, emu->cur_task->type_id + 1);
	emu_emit_prv(emu, 300, emu->cur_task->type_id + 1);
}

static void
emit_task_pause(struct ovni_emu *emu)
{
	emu_emit_prv(emu, 200, 0);
	emu_emit_prv(emu, 300, 0);
}

static void
emit_task_resume(struct ovni_emu *emu)
{
	emu_emit_prv(emu, 200, emu->cur_task->id + 1);
	emu_emit_prv(emu, 300, emu->cur_task->type_id + 1);
}

static void
emit_task_end(struct ovni_emu *emu)
{
	emu_emit_prv(emu, 200, 0);
	emu_emit_prv(emu, 300, 0);
}

static void
emit_task(struct ovni_emu *emu)
{
	switch(emu->cur_ev->header.value)
	{
		case 'c': emit_task_create(emu); break;
		case 'x': emit_task_execute(emu); break;
		case 'p': emit_task_pause(emu); break;
		case 'r': emit_task_resume(emu); break;
		case 'e': emit_task_end(emu); break;
		default:
			break;
	}
}

void
hook_emit_nosv(struct ovni_emu *emu)
{
	dbg("pre nosv\n");
	switch(emu->cur_ev->header.class)
	{
		case 'T': emit_task(emu); break;
		default:
			break;
	}
}

struct hook_entry pre_hooks[] = {
	{ "VTc", pre_task_create },
	{ "VTx", pre_task_create },
};
