/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define ENABLE_DEBUG

#include "model_nanos6.h"

#include "emu.h"
#include "loom.h"
#include "common.h"
#include "chan.h"

static const char chan_fmt_cpu_raw[] = "nanos6.cpu%ld.%s";
//static const char chan_fmt_cpu_run[] = "nanos6.cpu%ld.%s.run";
//static const char chan_fmt_cpu_act[] = "nanos6.cpu%ld.%s.act";
static const char chan_fmt_th_raw[] = "nanos6.thread%ld.%s.raw";
static const char chan_fmt_th_run[] = "nanos6.thread%ld.%s.run";
static const char chan_fmt_th_act[] = "nanos6.thread%ld.%s.act";

/* Private enums */
enum nanos6_chan_type {
	CH_TASKID = 0,
	CH_TYPE,
	CH_SUBSYSTEM,
	CH_RANK,
	CH_THREAD,
	CH_MAX,
};

enum nanos6_ss_state {
	ST_TASK_BODY = 1,
	ST_TASK_CREATING,
	ST_TASK_SUBMIT,
	ST_TASK_SPAWNING,
	ST_TASK_FOR,
	ST_SCHED_ADDING,
	ST_SCHED_PROCESSING,
	ST_SCHED_SERVING,
	ST_DEP_REG,
	ST_DEP_UNREG,
	ST_BLK_TASKWAIT,
	ST_BLK_WAITFOR,
	ST_BLK_BLOCKING,
	ST_BLK_UNBLOCKING,
	ST_ALLOCATING,
	ST_FREEING,
	ST_HANDLING_TASK,
	ST_WORKER_LOOP,
	ST_SWITCH_TO,
	ST_MIGRATE,
	ST_SUSPEND,
	ST_RESUME,

	/* Value 51 is broken in old Paraver */
	EV_SCHED_RECV = 60,
	EV_SCHED_SEND,
	EV_SCHED_SELF,
	EV_CPU_IDLE,
	EV_CPU_ACTIVE,
	EV_SIGNAL,
};

enum nanos6_thread_type {
	ST_TH_LEADER = 1,
	ST_TH_MAIN = 2,
	ST_TH_WORKER = 3,
	ST_TH_EXTERNAL = 4,
};

static const char *chan_name[] = {
	[CH_TASKID]    = "taskid",
	[CH_TYPE]      = "task_type",
	[CH_SUBSYSTEM] = "subsystem",
	[CH_RANK]      = "rank",
	[CH_THREAD]    = "thread_type",
};

static const char *th_track[] = {
	[CH_TASKID]    = "running",
	[CH_TYPE]      = "running",
	[CH_SUBSYSTEM] = "active",
	[CH_RANK]      = "running",
	[CH_THREAD]    = "none",
};

static const char *cpu_track[] = {
	[CH_TASKID]    = "running",
	[CH_TYPE]      = "running",
	[CH_SUBSYSTEM] = "running",
	[CH_RANK]      = "running",
	[CH_THREAD]    = "running",
};

static const int chan_stack[] = {
	[CH_SUBSYSTEM] = 1,
	[CH_THREAD] = 1,
};

static const int th_type[] = {
	[CH_TASKID]    = 35,
	[CH_TYPE]      = 36,
	[CH_SUBSYSTEM] = 37,
	[CH_RANK]      = 38,
	[CH_THREAD]    = 39,
};

static const int *cpu_type = th_type;

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
nanos6_probe(struct emu *emu)
{
	if (emu->system.nthreads == 0)
		return -1;

	return 0;
}

static int
init_chans(struct bay *bay, struct chan *chans, const char *fmt, int64_t gindex, int filtered)
{
	for (int i = 0; i < CH_MAX; i++) {
		struct chan *c = &chans[i];
		int type = (chan_stack[i] && !filtered) ? CHAN_STACK : CHAN_SINGLE;
		chan_init(c, type, fmt, gindex, chan_name[i]);

		if (bay_register(bay, c) != 0) {
			err("bay_register failed");
			return -1;
		}
	}

	return 0;
}

static int
init_cpu(struct bay *bay, struct cpu *syscpu)
{
	struct nanos6_cpu *cpu = calloc(1, sizeof(struct nanos6_cpu));
	if (cpu == NULL) {
		err("calloc failed:");
		return -1;
	}

	cpu->ch = calloc(CH_MAX, sizeof(struct chan));
	if (cpu->ch == NULL) {
		err("calloc failed:");
		return -1;
	}

	cpu->mux = calloc(CH_MAX, sizeof(struct mux));
	if (cpu->mux == NULL) {
		err("calloc failed:");
		return -1;
	}

	if (init_chans(bay, cpu->ch, chan_fmt_cpu_raw, syscpu->gindex, 1) != 0) {
		err("init_chans failed");
		return -1;
	}

	extend_set(&syscpu->ext, '6', cpu);
	return 0;
}

static int
init_thread(struct bay *bay, struct thread *systh)
{
	struct nanos6_thread *th = calloc(1, sizeof(struct nanos6_thread));
	if (th == NULL) {
		err("calloc failed:");
		return -1;
	}

	th->ch = calloc(CH_MAX, sizeof(struct chan));
	if (th->ch == NULL) {
		err("calloc failed:");
		return -1;
	}

	th->ch_run = calloc(CH_MAX, sizeof(struct chan));
	if (th->ch_run == NULL) {
		err("calloc failed:");
		return -1;
	}

	th->ch_act = calloc(CH_MAX, sizeof(struct chan));
	if (th->ch_act == NULL) {
		err("calloc failed:");
		return -1;
	}

	th->ch_out = calloc(CH_MAX, sizeof(struct chan *));
	if (th->ch_out == NULL) {
		err("calloc failed:");
		return -1;
	}

	th->mux_run = calloc(CH_MAX, sizeof(struct mux));
	if (th->mux_run == NULL) {
		err("calloc failed:");
		return -1;
	}

	th->mux_act = calloc(CH_MAX, sizeof(struct mux));
	if (th->mux_act == NULL) {
		err("calloc failed:");
		return -1;
	}

	if (init_chans(bay, th->ch, chan_fmt_th_raw, systh->gindex, 0) != 0) {
		err("init_chans failed");
		return -1;
	}

	if (init_chans(bay, th->ch_run, chan_fmt_th_run, systh->gindex, 1) != 0) {
		err("init_chans failed");
		return -1;
	}

	if (init_chans(bay, th->ch_act, chan_fmt_th_act, systh->gindex, 1) != 0) {
		err("init_chans failed");
		return -1;
	}


	th->task_stack.thread = systh;

	extend_set(&systh->ext, '6', th);

	return 0;
}

static int
init_proc(struct proc *sysproc)
{
	struct nanos6_proc *proc = calloc(1, sizeof(struct nanos6_proc));
	if (proc == NULL) {
		err("calloc failed:");
		return -1;
	}

	extend_set(&sysproc->ext, '6', proc);

	return 0;
}

static int
nanos6_create(struct emu *emu)
{
	struct system *sys = &emu->system;
	struct bay *bay = &emu->bay;

	for (struct cpu *c = sys->cpus; c; c = c->next) {
		if (init_cpu(bay, c) != 0) {
			err("init_cpu failed");
			return -1;
		}
	}

	for (struct thread *t = sys->threads; t; t = t->gnext) {
		if (init_thread(bay, t) != 0) {
			err("init_thread failed");
			return -1;
		}
	}

	for (struct proc *p = sys->procs; p; p = p->gnext) {
		if (init_proc(p) != 0) {
			err("init_proc failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_thread_mux(struct emu *emu, struct thread *thread)
{
	struct nanos6_thread *th = extend_get(&thread->ext, '6');
	for (int i = 0; i < CH_MAX; i++) {

		/* TODO: Let the thread take the select channel
		 * and build the mux as a tracking mode */
		struct chan *inp = &th->ch[i];
		struct chan *sel = &thread->chan[TH_CHAN_STATE];

		struct mux *mux_run = &th->mux_run[i];
		mux_select_func_t selrun = thread_select_running;
		if (mux_init(mux_run, &emu->bay, sel, &th->ch_run[i], selrun) != 0) {
			err("mux_init failed");
			return -1;
		}

		if (mux_add_input(mux_run, value_int64(0), inp) != 0) {
			err("mux_add_input failed");
			return -1;
		}

		struct mux *mux_act = &th->mux_act[i];
		mux_select_func_t selact = thread_select_active;
		if (mux_init(mux_act, &emu->bay, sel, &th->ch_act[i], selact) != 0) {
			err("mux_init failed");
			return -1;
		}

		if (mux_add_input(mux_act, value_int64(0), inp) != 0) {
			err("mux_add_input failed");
			return -1;
		}

		if (mux_act->ninputs != 1)
			die("expecting one input only");

		/* The tracking only sets the ch_out, but we keep both tracking
		 * updated as the CPU tracking channels may use them. */
		const char *tracking = th_track[i];
		if (strcmp(tracking, "running") == 0) {
			th->ch_out[i] = &th->ch_run[i];
		} else if (strcmp(tracking, "active") == 0) {
			th->ch_out[i] = &th->ch_act[i];
		} else {
			th->ch_out[i] = &th->ch[i];
		}

	}

	return 0;
}

static int
connect_thread_prv(struct emu *emu, struct thread *thread, struct prv *prv)
{
	struct nanos6_thread *th = extend_get(&thread->ext, '6');
	for (int i = 0; i < CH_MAX; i++) {
		struct chan *out = th->ch_out[i];
		long type = th_type[i];
		long row = thread->gindex;
		if (prv_register(prv, row, type, &emu->bay, out, PRV_DUP)) {
			err("prv_register failed");
			return -1;
		}
	}

	return 0;
}

static int
add_inputs_cpu_mux(struct emu *emu, struct mux *mux, int i)
{
	for (struct thread *t = emu->system.threads; t; t = t->gnext) {
		struct nanos6_thread *th = extend_get(&t->ext, '6');

		/* Choose input thread channel based on tracking mode */
		const char *tracking = cpu_track[i];
		struct chan *inp;
		if (strcmp(tracking, "running") == 0) {
			inp = &th->ch_run[i];
		} else if (strcmp(tracking, "active") == 0) {
			inp = &th->ch_act[i];
		} else {
			die("cpu tracking must be 'running' or 'active'");
		}

		if (mux_add_input(mux, value_int64(t->gindex), inp) != 0) {
			err("mux_add_input failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_cpu_mux(struct emu *emu, struct cpu *scpu)
{
	struct nanos6_cpu *cpu = extend_get(&scpu->ext, '6');
	for (int i = 0; i < CH_MAX; i++) {
		struct mux *mux = &cpu->mux[i];
		struct chan *out = &cpu->ch[i];
		const char *tracking = cpu_track[i];

		/* Choose select CPU channel based on tracking mode */
		struct chan *sel;
		if (strcmp(tracking, "running") == 0) {
			sel = &scpu->chan[CPU_CHAN_THRUN];
		} else if (strcmp(tracking, "active") == 0) {
			sel = &scpu->chan[CPU_CHAN_THACT];
		} else {
			die("cpu tracking must be 'running' or 'active'");
		}

		if (mux_init(mux, &emu->bay, sel, out, NULL) != 0) {
			err("mux_init failed");
			return -1;
		}

		if (add_inputs_cpu_mux(emu, mux, i) != 0) {
			err("add_inputs_cpu_mux failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_threads(struct emu *emu)
{
	struct system *sys = &emu->system;

	/* threads */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		if (connect_thread_mux(emu, t) != 0) {
			err("connect_thread_mux failed");
			return -1;
		}
	}

	/* Get thread PRV */
	struct pvt *pvt = recorder_find_pvt(&emu->recorder, "thread");
	if (pvt == NULL) {
		err("cannot find thread pvt");
		return -1;
	}
	struct prv *prv = pvt_get_prv(pvt);

	for (struct thread *t = sys->threads; t; t = t->gnext) {
		if (connect_thread_prv(emu, t, prv) != 0) {
			err("connect_thread_prv failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_cpu_prv(struct emu *emu, struct cpu *scpu, struct prv *prv)
{
	struct nanos6_cpu *cpu = extend_get(&scpu->ext, '6');
	for (int i = 0; i < CH_MAX; i++) {
		struct chan *out = &cpu->ch[i];
		long type = cpu_type[i];
		long row = scpu->gindex;
		if (prv_register(prv, row, type, &emu->bay, out, PRV_DUP)) {
			err("prv_register failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_cpus(struct emu *emu)
{
	struct system *sys = &emu->system;

	/* cpus */
	for (struct cpu *c = sys->cpus; c; c = c->next) {
		if (connect_cpu_mux(emu, c) != 0) {
			err("connect_cpu_mux failed");
			return -1;
		}
	}

	/* Get cpu PRV */
	struct pvt *pvt = recorder_find_pvt(&emu->recorder, "cpu");
	if (pvt == NULL) {
		err("cannot find cpu pvt");
		return -1;
	}
	struct prv *prv = pvt_get_prv(pvt);

	for (struct cpu *c = sys->cpus; c; c = c->next) {
		if (connect_cpu_prv(emu, c, prv) != 0) {
			err("connect_cpu_prv failed");
			return -1;
		}
	}

	return 0;
}


static int
nanos6_connect(struct emu *emu)
{
	if (connect_threads(emu) != 0) {
		err("connect_threads failed");
		return -1;
	}

	if (connect_cpus(emu) != 0) {
		err("connect_cpus failed");
		return -1;
	}

	return 0;
}

static int
simple(struct emu *emu)
{
	const int *entry = ss_table[emu->ev->c][emu->ev->v];
	int chind = entry[0];
	int action = entry[1];
	int st = entry[2];

	struct nanos6_thread *th = extend_get(&emu->thread->ext, '6');
	struct chan *ch = &th->ch[chind];

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

/* --------------------------- pre ------------------------------- */

static int
chan_task_stopped(struct emu *emu)
{
	struct nanos6_thread *th = extend_get(&emu->thread->ext, '6');

	struct value null = value_null();
	if (chan_set(&th->ch[CH_TASKID], null) != 0) {
		err("chan_set taskid failed");
		return -1;
	}

	if (chan_set(&th->ch[CH_TYPE], null) != 0) {
		err("chan_set type failed");
		return -1;
	}

	struct proc *proc = emu->proc;
	if (proc->rank >= 0) {
		if (chan_set(&th->ch[CH_RANK], null) != 0) {
			err("chan_set rank failed");
			return -1;
		}
	}

	return 0;
}

static int
chan_task_running(struct emu *emu, struct task *task)
{
	struct nanos6_thread *th = extend_get(&emu->thread->ext, '6');
	struct proc *proc = emu->proc;

	if (task->id == 0) {
		err("task id cannot be 0");
		return -1;
	}
	if (task->type->gid == 0) {
		err("task type gid cannot be 0");
		return -1;
	}

	if (chan_set(&th->ch[CH_TASKID], value_int64(task->id)) != 0) {
		err("chan_set taskid failed");
		return -1;
	}
	if (chan_set(&th->ch[CH_TYPE], value_int64(task->type->gid)) != 0) {
		err("chan_set type failed");
		return -1;
	}
	if (proc->rank >= 0) {
		struct value vrank = value_int64(proc->rank + 1);
		if (chan_set(&th->ch[CH_RANK], vrank) != 0) {
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
	struct nanos6_thread *th = extend_get(&emu->thread->ext, '6');

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
	if (chan_set(&th->ch[CH_TASKID], value_int64(next->id)) != 0) {
		err("chan_set taskid failed");
		return -1;
	}

	/* TODO: test when switching to another task with the same type. We
	 * should emit the same type state value as previous task. */
	if (chan_set(&th->ch[CH_TYPE], value_int64(next->type->gid)) != 0) {
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

	struct nanos6_thread *th = extend_get(&emu->thread->ext, '6');
	struct nanos6_proc *proc = extend_get(&emu->proc->ext, '6');

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
enforce_task_rules(struct emu *emu, char tr,
		struct task *prev, struct task *next)

{
	UNUSED(prev);

	if (tr != 'x' && tr != 'X')
		return 0;

	/* If a task has just entered the running state, it must show
	 * the running task body subsystem */

	if (next->state != TASK_ST_RUNNING) {
		err("task not in running state on begin");
		return -1;
	}

	struct nanos6_thread *th = extend_get(&emu->thread->ext, '6');
	struct value ss;
	if (chan_read(&th->ch[CH_SUBSYSTEM], &ss) != 0) {
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
	struct nanos6_thread *th = extend_get(&emu->thread->ext, '6');
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

	if (enforce_task_rules(emu, tr, prev, next) != 0) {
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

	struct nanos6_proc *proc = extend_get(&emu->proc->ext, '6');
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

	struct nanos6_proc *proc = extend_get(&emu->proc->ext, '6');
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
			return simple(emu);
		case 'T':
			return pre_task(emu);
		case 'Y':
			return pre_type(emu);
		default:
			err("unknown Nanos6 event category");
//			return -1;
	}

	/* Not reached */
	return 0;
}

static int
nanos6_event(struct emu *emu)
{
	if (emu->ev->m != model_nanos6.model) {
		err("unexpected event model %c\n", emu->ev->m);
		return -1;
	}

	dbg("got nanos6 event %s", emu->ev->mcv);
	if (process_ev(emu) != 0) {
		err("error processing Nanos6 event");
		return -1;
	}

	//check_affinity(emu);

	return 0;
}

struct model_spec model_nanos6 = {
	.name = "nanos6",
	.model = '6',
	.create  = nanos6_create,
	.connect = nanos6_connect,
	.event   = nanos6_event,
	.probe   = nanos6_probe,
};
