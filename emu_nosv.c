#include "ovni.h"
#include "ovni_trace.h"
#include "emu.h"
#include "uthash.h"
#include <assert.h>

struct nosv_task *tasks = NULL;

struct hook_entry {
	char id[4];
	void (*hook)(struct ovni_emu *);
};

static void
pre_task_create(struct ovni_emu *emu)
{
	struct nosv_task *task, *p = NULL;
	
	task = malloc(sizeof(*task));

	if(task == NULL)
	{
		perror("malloc");
		abort();
	}

	task->id = emu->cur_ev->payload.i32[0];
	task->state = TASK_ST_CREATED;

	/* Ensure the task id is new */
	HASH_FIND_INT(tasks, &task->id, p);

	if(p != NULL)
	{
		err("A task with id %d already exists\n", task->id);
		abort();
	}

	/* Add the new task to the hash table */
	HASH_ADD_INT(tasks, id, task);

	emu->cur_task = task;

	dbg("new task created id=%d\n", task->id);
}

static void
pre_task_execute(struct ovni_emu *emu)
{
	struct nosv_task *task;
	int taskid;

	taskid = emu->cur_ev->payload.i32[0];

	HASH_FIND_INT(tasks, &taskid, task);

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

	HASH_FIND_INT(tasks, &taskid, task);

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

	HASH_FIND_INT(tasks, &taskid, task);

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

	/* Ensure the task id is new */
	HASH_FIND_INT(tasks, &taskid, task);

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
	switch(emu->cur_ev->value)
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

void
hook_pre_nosv(struct ovni_emu *emu)
{
	dbg("pre nosv\n");
	switch(emu->cur_ev->class)
	{
		case 'T': pre_task(emu); break;
		default:
			  break;
	}
}

/* --------------------------- views ------------------------------- */

static void
emit_prv(struct ovni_emu *emu, int type, int val)
{
	printf("2:0:1:1:%d:%ld:%d:%d\n",
			emu->cur_thread->cpu->cpu_id + 2,
			emu->delta_time,
			type, val);
}

static void
emit_task_create(struct ovni_emu *emu)
{
	//emit_prv(emu, 200, emu->cur_task->id);
}

static void
emit_task_execute(struct ovni_emu *emu)
{
	emit_prv(emu, 200, emu->cur_task->id + 1);
}

static void
emit_task_pause(struct ovni_emu *emu)
{
	emit_prv(emu, 200, 0);
}

static void
emit_task_resume(struct ovni_emu *emu)
{
	emit_prv(emu, 200, emu->cur_task->id + 1);
}

static void
emit_task_end(struct ovni_emu *emu)
{
	emit_prv(emu, 200, 0);
}

static void
emit_task(struct ovni_emu *emu)
{
	switch(emu->cur_ev->value)
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
hook_view_nosv(struct ovni_emu *emu)
{
	dbg("pre nosv\n");
	switch(emu->cur_ev->class)
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
