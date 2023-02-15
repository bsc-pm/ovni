#include "nanos6_priv.h"

enum { PUSH = 1, POP = 2, IGN = 3 };

#define CHSS CH_SUBSYSTEM
#define CHTH CH_THREAD

static const int ss_table[256][256][3] = {
	['W'] = {
		['['] = { CHSS, PUSH, ST_WORKER_LOOP },
		[']'] = { CHSS, POP,  ST_WORKER_LOOP },
		['t'] = { CHSS, PUSH, ST_HANDLING_TASK },
		['T'] = { CHSS, POP,  ST_HANDLING_TASK },
		['w'] = { CHSS, PUSH, ST_SWITCH_TO },
		['W'] = { CHSS, POP,  ST_SWITCH_TO },
		['m'] = { CHSS, PUSH, ST_MIGRATE },
		['M'] = { CHSS, POP,  ST_MIGRATE },
		['s'] = { CHSS, PUSH, ST_SUSPEND },
		['S'] = { CHSS, POP,  ST_SUSPEND },
		['r'] = { CHSS, PUSH, ST_RESUME },
		['R'] = { CHSS, POP,  ST_RESUME },
		['*'] = { CHSS, IGN,  -1 },
	},
	['C'] = {
		['['] = { CHSS, PUSH, ST_TASK_CREATING },
		[']'] = { CHSS, POP,  ST_TASK_CREATING },
	},
	['U'] = {
		['['] = { CHSS, PUSH, ST_TASK_SUBMIT },
		[']'] = { CHSS, POP,  ST_TASK_SUBMIT },
	},
	['F'] = {
		['['] = { CHSS, PUSH, ST_TASK_SPAWNING },
		[']'] = { CHSS, POP,  ST_TASK_SPAWNING },
	},
	['O'] = {
		['['] = { CHSS, PUSH, ST_TASK_FOR },
		[']'] = { CHSS, POP,  ST_TASK_FOR },
	},
	['t'] = {
		['['] = { CHSS, PUSH, ST_TASK_BODY },
		[']'] = { CHSS, POP,  ST_TASK_BODY },
	},
	['M'] = {
		['a'] = { CHSS, PUSH, ST_ALLOCATING },
		['A'] = { CHSS, POP,  ST_ALLOCATING },
		['f'] = { CHSS, PUSH, ST_FREEING },
		['F'] = { CHSS, POP,  ST_FREEING },
	},
	['D'] = {
		['r'] = { CHSS, PUSH, ST_DEP_REG },
		['R'] = { CHSS, POP,  ST_DEP_REG },
		['u'] = { CHSS, PUSH, ST_DEP_UNREG },
		['U'] = { CHSS, POP,  ST_DEP_UNREG },
	},
	['S'] = {
		['['] = { CHSS, PUSH, ST_SCHED_SERVING },
		[']'] = { CHSS, POP,  ST_SCHED_SERVING },
		['a'] = { CHSS, PUSH, ST_SCHED_ADDING },
		['A'] = { CHSS, POP,  ST_SCHED_ADDING },
		['p'] = { CHSS, PUSH, ST_SCHED_PROCESSING },
		['P'] = { CHSS, POP,  ST_SCHED_PROCESSING },
		['@'] = { CHSS, IGN,  -1 },
		['r'] = { CHSS, IGN,  -1 },
		['s'] = { CHSS, IGN,  -1 },
	},
	['B'] = {
		['b'] = { CHSS, PUSH, ST_BLK_BLOCKING },
		['B'] = { CHSS, POP,  ST_BLK_BLOCKING },
		['u'] = { CHSS, PUSH, ST_BLK_UNBLOCKING },
		['U'] = { CHSS, POP,  ST_BLK_UNBLOCKING },
		['w'] = { CHSS, PUSH, ST_BLK_TASKWAIT },
		['W'] = { CHSS, POP,  ST_BLK_TASKWAIT },
		['f'] = { CHSS, PUSH, ST_BLK_WAITFOR },
		['F'] = { CHSS, POP,  ST_BLK_WAITFOR },
	},
	['H'] = {
		['e'] = { CHTH, PUSH, ST_TH_EXTERNAL },
		['E'] = { CHTH, POP,  ST_TH_EXTERNAL },
		['w'] = { CHTH, PUSH, ST_TH_WORKER },
		['W'] = { CHTH, POP,  ST_TH_WORKER },
		['l'] = { CHTH, PUSH, ST_TH_LEADER },
		['L'] = { CHTH, POP,  ST_TH_LEADER },
		['m'] = { CHTH, PUSH, ST_TH_MAIN },
		['M'] = { CHTH, POP,  ST_TH_MAIN },
	},
};

static int
simple(struct emu *emu)
{
	const int *entry = ss_table[emu->ev->c][emu->ev->v];
	int chind = entry[0];
	int action = entry[1];
	int st = entry[2];

	struct nanos6_thread *th = EXT(emu->thread, '6');
	struct chan *ch = &th->m.ch[chind];

	if (action == PUSH) {
		return chan_push(ch, value_int64(st));
	} else if (action == POP) {
		return chan_pop(ch, value_int64(st));
	} else if (action == IGN) {
		return 0; /* do nothing */
	} else {
		err("unknown Nanos6 subsystem event");
		return -1;
	}

	return 0;
}

static int
chan_task_stopped(struct emu *emu)
{
	struct nanos6_thread *th = EXT(emu->thread, '6');

	struct value null = value_null();
	if (chan_set(&th->m.ch[CH_TASKID], null) != 0) {
		err("chan_set taskid failed");
		return -1;
	}

	if (chan_set(&th->m.ch[CH_TYPE], null) != 0) {
		err("chan_set type failed");
		return -1;
	}

	struct proc *proc = emu->proc;
	if (proc->rank >= 0) {
		if (chan_set(&th->m.ch[CH_RANK], null) != 0) {
			err("chan_set rank failed");
			return -1;
		}
	}

	return 0;
}

static int
chan_task_running(struct emu *emu, struct task *task)
{
	struct nanos6_thread *th = EXT(emu->thread, '6');
	struct proc *proc = emu->proc;

	if (task->id == 0) {
		err("task id cannot be 0");
		return -1;
	}
	if (task->type->gid == 0) {
		err("task type gid cannot be 0");
		return -1;
	}

	if (chan_set(&th->m.ch[CH_TASKID], value_int64(task->id)) != 0) {
		err("chan_set taskid failed");
		return -1;
	}
	if (chan_set(&th->m.ch[CH_TYPE], value_int64(task->type->gid)) != 0) {
		err("chan_set type failed");
		return -1;
	}
	if (proc->rank >= 0) {
		struct value vrank = value_int64(proc->rank + 1);
		if (chan_set(&th->m.ch[CH_RANK], vrank) != 0) {
			err("chan_set rank failed");
			return -1;
		}
	}

	return 0;
}

static int
chan_task_switch(struct emu *emu,
		struct task *prev, struct task *next)
{
	struct nanos6_thread *th = EXT(emu->thread, '6');

	if (!prev || !next) {
		err("cannot switch to or from a NULL task");
		return -1;
	}

	if (prev == next) {
		err("cannot switch to the same task");
		return -1;
	}

	if (next->id == 0) {
		err("next task id cannot be 0");
		return -1;
	}

	if (next->type->gid == 0) {
		err("next task type id cannot be 0");
		return -1;
	}

	if (prev->thread != next->thread) {
		err("cannot switch to a task of another thread");
		return -1;
	}

	/* No need to change the rank as we will switch to tasks from
	 * same thread */
	if (chan_set(&th->m.ch[CH_TASKID], value_int64(next->id)) != 0) {
		err("chan_set taskid failed");
		return -1;
	}

	/* TODO: test when switching to another task with the same type. We
	 * should emit the same type state value as previous task. */
	if (chan_set(&th->m.ch[CH_TYPE], value_int64(next->type->gid)) != 0) {
		err("chan_set type failed");
		return -1;
	}

	return 0;
}

static int
update_task_state(struct emu *emu)
{
	if (emu->ev->payload_size < 4) {
		err("missing task id in payload");
		return -1;
	}

	uint32_t task_id = emu->ev->payload->u32[0];

	struct nanos6_thread *th = EXT(emu->thread, '6');
	struct nanos6_proc *proc = EXT(emu->proc, '6');

	struct task_info *info = &proc->task_info;
	struct task_stack *stack = &th->task_stack;

	struct task *task = task_find(info->tasks, task_id);

	if (task == NULL) {
		err("cannot find task with id %u", task_id);
		return -1;
	}

	int ret = 0;
	switch (emu->ev->v) {
		case 'x':
			ret = task_execute(stack, task);
			break;
		case 'e':
			ret = task_end(stack, task);
			break;
		case 'p':
			ret = task_pause(stack, task);
			break;
		case 'r':
			ret = task_resume(stack, task);
			break;
		default:
			err("unexpected Nanos6 task event");
			return -1;
	}

	if (ret != 0) {
		err("cannot change task state");
		return -1;
	}

	return 0;
}

static int
expand_transition_value(struct emu *emu, int was_running, int runs_now, char *tr_p)
{
	char tr = emu->ev->v;

	/* Ensure we don't clobber the value */
	if (tr == 'X' || tr == 'E') {
		err("unexpected event value %c", tr);
		return -1;
	}

	/* Modify the event value to detect nested transitions */
	if (tr == 'x' && was_running)
		tr = 'X'; /* Execute a new nested task */
	else if (tr == 'e' && runs_now)
		tr = 'E'; /* End a nested task */

	*tr_p = tr;
	return 0;
}

static int
update_task_channels(struct emu *emu,
		char tr, struct task *prev, struct task *next)
{
	int ret = 0;
	switch (tr) {
		case 'x':
		case 'r':
			ret = chan_task_running(emu, next);
			break;
		case 'e':
		case 'p':
			ret = chan_task_stopped(emu);
			break;
			/* Additional nested transitions */
		case 'X':
		case 'E':
			ret = chan_task_switch(emu, prev, next);
			break;
		default:
			err("unexpected transition value %c", tr);
			return -1;
	}

	if (ret != 0) {
		err("cannot update task channels");
		return -1;
	}

	return 0;
}

static int
enforce_task_rules(struct emu *emu, char tr, struct task *next)
{
	if (tr != 'x' && tr != 'X')
		return 0;

	/* If a task has just entered the running state, it must show
	 * the running task body subsystem */

	if (next->state != TASK_ST_RUNNING) {
		err("task not in running state on begin");
		return -1;
	}

	struct nanos6_thread *th = EXT(emu->thread, '6');
	struct value ss;
	if (chan_read(&th->m.ch[CH_SUBSYSTEM], &ss) != 0) {
		err("chan_read failed");
		return -1;
	}

	if (ss.type == VALUE_INT64 && ss.i != ST_TASK_BODY) {
		err("wrong subsystem state on task begin");
		//return -1;
		return 0; // FIXME
	}

	return 0;
}

static int
update_task(struct emu *emu)
{
	struct nanos6_thread *th = EXT(emu->thread, '6');
	struct task_stack *stack = &th->task_stack;

	struct task *prev = task_get_running(stack);

	/* Update the emulator state, but don't modify the channels */
	if (update_task_state(emu) != 0) {
		err("update_task_state failed");
		return -1;
	}

	struct task *next = task_get_running(stack);

	int was_running = (prev != NULL);
	int runs_now = (next != NULL);
	char tr;
	if (expand_transition_value(emu, was_running, runs_now, &tr) != 0) {
		err("expand_transition_value failed");
		return -1;
	}

	/* Update the task related channels now */
	update_task_channels(emu, tr, prev, next);

	if (enforce_task_rules(emu, tr, next) != 0) {
		err("enforce_task_rules failed");
		return -1;
	}

	return 0;
}

static int
create_task(struct emu *emu)
{
	if (emu->ev->payload_size != 8) {
		err("unexpected payload size");
		return -1;
	}

	uint32_t task_id = emu->ev->payload->u32[0];
	uint32_t type_id = emu->ev->payload->u32[1];

	struct nanos6_proc *proc = EXT(emu->proc, '6');
	struct task_info *info = &proc->task_info;

	if (task_create(info, type_id, task_id) != 0) {
		err("task_create failed");
		return -1;
	}

	return 0;
}

static int
pre_task(struct emu *emu)
{
	int ret = 0;
	switch (emu->ev->v) {
		case 'C':
			err("warning: got old 6TC event, ignoring");
			break;
		case 'c':
			ret = create_task(emu);
			break;
		case 'x':
		case 'e':
		case 'r':
		case 'p':
			ret = update_task(emu);
			break;
		default:
			err("unexpected Nanos6 task event value");
			return -1;
	}

	if (ret != 0) {
		err("cannot update task state");
		return -1;
	}

	return 0;
}

static int
pre_type(struct emu *emu)
{
	uint8_t value = emu->ev->v;

	if (value != 'c') {
		err("unexpected event value %c", value);
		return -1;
	}

	if (!emu->ev->is_jumbo) {
		err("expecting a jumbo event");
		return -1;
	}

	const uint8_t *data = &emu->ev->payload->jumbo.data[0];
	uint32_t typeid = *(uint32_t *) data;
	data += 4;

	const char *label = (const char *) data;

	struct nanos6_proc *proc = EXT(emu->proc, '6');
	struct task_info *info = &proc->task_info;

	if (task_type_create(info, typeid, label) != 0) {
		err("task_type_create failed");
		return -1;
	}

	return 0;
}

static int
process_ev(struct emu *emu)
{
	if (!emu->thread->is_active) {
		err("current thread %d not active", emu->thread->tid);
		return -1;
	}

	switch (emu->ev->c) {
		case 'C':
		case 'S':
		case 'U':
		case 'F':
		case 'O':
		case 't':
		case 'H':
		case 'D':
		case 'B':
		case 'W':
		case 'M':
			return simple(emu);
		case 'T':
			return pre_task(emu);
		case 'Y':
			return pre_type(emu);
		default:
			err("unknown Nanos6 event category");
			return -1;
	}

	/* Not reached */
	return 0;
}

int
nanos6_event(struct emu *emu)
{
	if (emu->ev->m != '6') {
		err("unexpected event model %c\n", emu->ev->m);
		return -1;
	}

	dbg("got nanos6 event %s", emu->ev->mcv);
	if (process_ev(emu) != 0) {
		err("error processing Nanos6 event");
		return -1;
	}

	return 0;
}