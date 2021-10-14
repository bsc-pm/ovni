#include <assert.h>
#include "uthash.h"

#include "ovni.h"
#include "ovni_trace.h"
#include "emu.h"
#include "prv.h"
#include "chan.h"

/* --------------------------- init ------------------------------- */

void
hook_init_nosv(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_cpu *cpu;
	struct ovni_trace *trace;
	int i, row, type;
	FILE *prv_th, *prv_cpu;
	int64_t *clock;

	clock = &emu->delta_time;
	prv_th = emu->prv_thread;
	prv_cpu = emu->prv_cpu;
	trace = &emu->trace;

	/* Init the channels in all threads */
	for(i=0; i<emu->total_nthreads; i++)
	{
		th = emu->global_thread[i];
		row = th->gindex + 1;

		chan_th_init(th, CHAN_NOSV_TASKID, CHAN_TRACK_TH_RUNNING, row, prv_th, clock);
		chan_enable(&th->chan[CHAN_NOSV_TASKID], 1);
		chan_set(&th->chan[CHAN_NOSV_TASKID], 0);
		chan_enable(&th->chan[CHAN_NOSV_TASKID], 0);

		chan_th_init(th, CHAN_NOSV_TYPEID, CHAN_TRACK_TH_RUNNING, row, prv_th, clock);
		chan_th_init(th, CHAN_NOSV_APPID, CHAN_TRACK_TH_RUNNING, row, prv_th, clock);

		/* We allow threads to emit subsystem events in cooling and
		 * warming states as well, as they may be allocating memory.
		 * However, these information won't be presented in the CPU
		 * channel, as it only shows the thread in the running state */
		chan_th_init(th, CHAN_NOSV_SUBSYSTEM, CHAN_TRACK_TH_UNPAUSED, row, prv_th, clock);
	}

	/* Init the nosv channels in all cpus */
	for(i=0; i<emu->total_ncpus; i++)
	{
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;

		chan_cpu_init(cpu, CHAN_NOSV_TASKID, CHAN_TRACK_TH_RUNNING, row, prv_cpu, clock);
		chan_enable(&cpu->chan[CHAN_NOSV_TASKID], 1);
		chan_set(&cpu->chan[CHAN_NOSV_TASKID], 0);
		chan_enable(&cpu->chan[CHAN_NOSV_TASKID], 0);

		chan_cpu_init(cpu, CHAN_NOSV_TYPEID, CHAN_TRACK_TH_RUNNING, row, prv_cpu, clock);
		chan_cpu_init(cpu, CHAN_NOSV_APPID, CHAN_TRACK_TH_RUNNING, row, prv_cpu, clock);
		chan_cpu_init(cpu, CHAN_NOSV_SUBSYSTEM, CHAN_TRACK_TH_RUNNING, row, prv_cpu, clock);
	}
}

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
pre_task_running(struct ovni_emu *emu, struct nosv_task *task)
{
	struct ovni_ethread *th;
	struct ovni_eproc *proc;

	th = emu->cur_thread;
	proc = emu->cur_proc;

	chan_set(&th->chan[CHAN_NOSV_TASKID], task->id + 1);
	chan_set(&th->chan[CHAN_NOSV_TYPEID], task->type_id + 1);
	chan_set(&th->chan[CHAN_NOSV_APPID], proc->appid + 1);

	chan_push(&th->chan[CHAN_NOSV_SUBSYSTEM], ST_TASK_RUNNING);
}

static void
pre_task_not_running(struct ovni_emu *emu, struct nosv_task *task)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;

	chan_set(&th->chan[CHAN_NOSV_TASKID], 0);
	chan_set(&th->chan[CHAN_NOSV_TYPEID], 0);
	chan_set(&th->chan[CHAN_NOSV_APPID], 0);

	chan_pop(&th->chan[CHAN_NOSV_SUBSYSTEM], ST_TASK_RUNNING);
}

static void
pre_task(struct ovni_emu *emu)
{
	struct nosv_task *task;

	task = emu->cur_task;

	switch(emu->cur_ev->header.value)
	{
		case 'c': pre_task_create(emu); break;
		case 'x': pre_task_execute(emu); break;
		case 'e': pre_task_end(emu); break;
		case 'p': pre_task_pause(emu); break;
		case 'r': pre_task_resume(emu); break;
		default:
			  abort();
	}

	switch(emu->cur_ev->header.value)
	{
		case 'x':
		case 'r':
			pre_task_running(emu, task);
			break;
		case 'p':
		case 'e':
			pre_task_not_running(emu, task);
			break;
		case 'c':
		default:
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

static void
pre_sched(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case 'h':
			chan_push(chan_th, ST_SCHED_HUNGRY);
			break;
		case 'f': /* Fill: no longer hungry */
			chan_pop(chan_th, ST_SCHED_HUNGRY);
			break;
		case '[': /* Server enter */
			chan_push(chan_th, ST_SCHED_SERVING);
			break;
		case ']': /* Server exit */
			chan_pop(chan_th, ST_SCHED_SERVING);
			break;
		case '@':
			chan_ev(chan_th, EV_SCHED_SELF);
			break;
		case 'r':
			chan_ev(chan_th, EV_SCHED_RECV);
			break;
		case 's':
			chan_ev(chan_th, EV_SCHED_SEND);
			break;
		default:
			break;
	}
}

static void
pre_ss(struct ovni_emu *emu, int st)
{
	struct ovni_ethread *th;
	struct ovni_chan *chan_th;

	th = emu->cur_thread;
	chan_th = &th->chan[CHAN_NOSV_SUBSYSTEM];

	switch(emu->cur_ev->header.value)
	{
		case '[': 
			chan_push(chan_th, st);
			break;
		case ']':
			chan_pop(chan_th, st);
			break;
		default:
			err("unexpected value '%c' (expecting '[' or ']')\n",
					emu->cur_ev->header.value);
			abort();
	}
}

void
hook_pre_nosv(struct ovni_emu *emu)
{
	assert(emu->cur_ev->header.model == 'V');

	switch(emu->cur_ev->header.category)
	{
		case 'T': pre_task(emu); break;
		case 'Y': pre_type(emu); break;
		case 'S': pre_sched(emu); break;
		case 'U': pre_ss(emu, ST_SCHED_SUBMITTING); break;
		case 'M': pre_ss(emu, ST_MEM_ALLOCATING); break;
		case 'P': pre_ss(emu, ST_PAUSE); break;
		case 'I': pre_ss(emu, ST_YIELD); break;
		case 'W': pre_ss(emu, ST_WAITFOR); break;
		case 'D': pre_ss(emu, ST_SCHEDPOINT); break;
		case 'C': pre_ss(emu, ST_NOSV_CODE); break;
		default:
			break;
	}
}
