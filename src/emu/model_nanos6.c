/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define ENABLE_DEBUG

#include "model_nanos6.h"

#include "emu.h"
#include "loom.h"
#include "common.h"
#include "chan.h"

/* Raw channels */
static const char chan_cpu_fmt[] = "nanos6.cpu%ld.%s.raw";
static const char chan_th_fmt[] = "nanos6.thread%ld.%s.raw";

/* Channels filtered by tracking */
//static const char chan_fcpu_fmt[] = "nanos6.cpu%ld.%s.filtered";
static const char chan_fth_fmt[] = "nanos6.thread%ld.%s.filtered";

static const char *chan_name[] = {
	[NANOS6_CHAN_TASKID]    = "taskid",
	[NANOS6_CHAN_TYPE]      = "task_type",
	[NANOS6_CHAN_SUBSYSTEM] = "subsystem",
	[NANOS6_CHAN_RANK]      = "rank",
	[NANOS6_CHAN_THREAD]    = "thread_type",
};

static const char *th_track[] = {
	[NANOS6_CHAN_TASKID]    = "running",
	[NANOS6_CHAN_TYPE]      = "running",
	[NANOS6_CHAN_SUBSYSTEM] = "active",
	[NANOS6_CHAN_RANK]      = "running",
	[NANOS6_CHAN_THREAD]    = "none",
};

static const int chan_stack[] = {
	[NANOS6_CHAN_SUBSYSTEM] = 1,
	[NANOS6_CHAN_THREAD] = 1,
};

static const int th_type[] = {
	[NANOS6_CHAN_TASKID]    = 35,
	[NANOS6_CHAN_TYPE]      = 36,
	[NANOS6_CHAN_SUBSYSTEM] = 37,
	[NANOS6_CHAN_RANK]      = 38,
	[NANOS6_CHAN_THREAD]    = 39,
};

enum { PUSH = 1, POP = 2, IGN = 3 };

#define CHSS NANOS6_CHAN_SUBSYSTEM
#define CHTT NANOS6_CHAN_THREAD

static const int ss_table[256][256][3] = {
['W'] = {
	['['] = { CHSS, PUSH, ST_NANOS6_WORKER_LOOP },
	[']'] = { CHSS, POP,  ST_NANOS6_WORKER_LOOP },
	['t'] = { CHSS, PUSH, ST_NANOS6_HANDLING_TASK },
	['T'] = { CHSS, POP,  ST_NANOS6_HANDLING_TASK },
	['w'] = { CHSS, PUSH, ST_NANOS6_SWITCH_TO },
	['W'] = { CHSS, POP,  ST_NANOS6_SWITCH_TO },
	['m'] = { CHSS, PUSH, ST_NANOS6_MIGRATE },
	['M'] = { CHSS, POP,  ST_NANOS6_MIGRATE },
	['s'] = { CHSS, PUSH, ST_NANOS6_SUSPEND },
	['S'] = { CHSS, POP,  ST_NANOS6_SUSPEND },
	['r'] = { CHSS, PUSH, ST_NANOS6_RESUME },
	['R'] = { CHSS, POP,  ST_NANOS6_RESUME },
	['*'] = { CHSS, IGN,  -1 },
},
['C'] = {
	['['] = { CHSS, PUSH, ST_NANOS6_TASK_CREATING },
	[']'] = { CHSS, POP,  ST_NANOS6_TASK_CREATING },
},
['U'] = {
	['['] = { CHSS, PUSH, ST_NANOS6_TASK_SUBMIT },
	[']'] = { CHSS, POP,  ST_NANOS6_TASK_SUBMIT },
},
['F'] = {
	['['] = { CHSS, PUSH, ST_NANOS6_TASK_SPAWNING },
	[']'] = { CHSS, POP,  ST_NANOS6_TASK_SPAWNING },
},
['O'] = {
	['['] = { CHSS, PUSH, ST_NANOS6_TASK_FOR },
	[']'] = { CHSS, POP,  ST_NANOS6_TASK_FOR },
},
['t'] = {
	['['] = { CHSS, PUSH, ST_NANOS6_TASK_BODY },
	[']'] = { CHSS, POP,  ST_NANOS6_TASK_BODY },
},
['M'] = {
	['a'] = { CHSS, PUSH, ST_NANOS6_ALLOCATING },
	['A'] = { CHSS, POP,  ST_NANOS6_ALLOCATING },
	['f'] = { CHSS, PUSH, ST_NANOS6_FREEING },
	['F'] = { CHSS, POP,  ST_NANOS6_FREEING },
},
['D'] = {
	['r'] = { CHSS, PUSH, ST_NANOS6_DEP_REG },
	['R'] = { CHSS, POP,  ST_NANOS6_DEP_REG },
	['u'] = { CHSS, PUSH, ST_NANOS6_DEP_UNREG },
	['U'] = { CHSS, POP,  ST_NANOS6_DEP_UNREG },
},
['S'] = {
	['['] = { CHSS, PUSH, ST_NANOS6_SCHED_SERVING },
	[']'] = { CHSS, POP,  ST_NANOS6_SCHED_SERVING },
	['a'] = { CHSS, PUSH, ST_NANOS6_SCHED_ADDING },
	['A'] = { CHSS, POP,  ST_NANOS6_SCHED_ADDING },
	['p'] = { CHSS, PUSH, ST_NANOS6_SCHED_PROCESSING },
	['P'] = { CHSS, POP,  ST_NANOS6_SCHED_PROCESSING },
	['@'] = { CHSS, IGN,  -1 },
	['r'] = { CHSS, IGN,  -1 },
	['s'] = { CHSS, IGN,  -1 },
},
['B'] = {
	['b'] = { CHSS, PUSH, ST_NANOS6_BLK_BLOCKING },
	['B'] = { CHSS, POP,  ST_NANOS6_BLK_BLOCKING },
	['u'] = { CHSS, PUSH, ST_NANOS6_BLK_UNBLOCKING },
	['U'] = { CHSS, POP,  ST_NANOS6_BLK_UNBLOCKING },
	['w'] = { CHSS, PUSH, ST_NANOS6_BLK_TASKWAIT },
	['W'] = { CHSS, POP,  ST_NANOS6_BLK_TASKWAIT },
	['f'] = { CHSS, PUSH, ST_NANOS6_BLK_WAITFOR },
	['F'] = { CHSS, POP,  ST_NANOS6_BLK_WAITFOR },
},
['H'] = {
	['e'] = { CHTT, PUSH, ST_NANOS6_TH_EXTERNAL },
	['E'] = { CHTT, POP,  ST_NANOS6_TH_EXTERNAL },
	['w'] = { CHTT, PUSH, ST_NANOS6_TH_WORKER },
	['W'] = { CHTT, POP,  ST_NANOS6_TH_WORKER },
	['l'] = { CHTT, PUSH, ST_NANOS6_TH_LEADER },
	['L'] = { CHTT, POP,  ST_NANOS6_TH_LEADER },
	['m'] = { CHTT, PUSH, ST_NANOS6_TH_MAIN },
	['M'] = { CHTT, POP,  ST_NANOS6_TH_MAIN },
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
init_chans(struct bay *bay, struct chan chans[], const char *fmt, int64_t gindex, int filtered)
{
	for (int i = 0; i < NANOS6_CHAN_MAX; i++) {
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
nanos6_create(struct emu *emu)
{
	struct system *sys = &emu->system;
	struct bay *bay = &emu->bay;

	/* Create nanos6 cpu data */
	struct nanos6_cpu *cpus = calloc(sys->ncpus, sizeof(*cpus));
	if (cpus == NULL) {
		err("calloc failed:");
		return -1;
	}

	for (struct cpu *c = sys->cpus; c; c = c->next) {
		struct nanos6_cpu *cpu = &cpus[c->gindex];
		if (init_chans(bay, cpu->chans, chan_cpu_fmt, c->gindex, 0) != 0) {
			err("init_chans failed");
			return -1;
		}
		extend_set(&c->ext, model_nanos6.model, cpu);
	}

	/* Create nanos6 thread data */
	struct nanos6_thread *threads = calloc(sys->nthreads, sizeof(*threads));
	if (threads == NULL) {
		err("calloc failed:");
		return -1;
	}

	for (struct thread *t = sys->threads; t; t = t->gnext) {
		struct nanos6_thread *th = &threads[t->gindex];
		if (init_chans(bay, th->chans, chan_th_fmt, t->gindex, 0) != 0) {
			err("init_chans failed");
			return -1;
		}
		if (init_chans(bay, th->fchans, chan_fth_fmt, t->gindex, 1) != 0) {
			err("init_chans failed");
			return -1;
		}
		extend_set(&t->ext, model_nanos6.model, th);
	}

	return 0;
}

static int
connect_thread_mux(struct emu *emu, struct thread *thread)
{
	struct nanos6_thread *th = extend_get(&thread->ext, '6');
	for (int i = 0; i < NANOS6_CHAN_MAX; i++) {
		struct mux *mux = &th->muxers[i];

		const char *tracking = th_track[i];
		mux_select_func_t selfun;

		struct chan *inp = &th->chans[i];

		/* TODO: Let the thread take the select channel
		 * and build the mux as a tracking mode */
		struct chan *sel = &thread->chan[TH_CHAN_STATE];

		if (strcmp(tracking, "running") == 0) {
			selfun = thread_select_running;
		} else if (strcmp(tracking, "active") == 0) {
			selfun = thread_select_active;
		} else {
			th->ochans[i] = inp;
			/* No tracking */
			continue;
		}

		struct chan *out = &th->fchans[i];
		th->ochans[i] = out;

		if (mux_init(mux, &emu->bay, sel, out, selfun) != 0) {
			err("mux_init failed");
			return -1;
		}

		if (mux_add_input(mux, value_int64(0), inp) != 0) {
			err("mux_add_input failed");
			return -1;
		}

		/* Connect to prv output */
	}

	return 0;
}

static int
connect_thread_prv(struct emu *emu, struct thread *thread, struct prv *prv)
{
	struct nanos6_thread *th = extend_get(&thread->ext, '6');
	for (int i = 0; i < NANOS6_CHAN_MAX; i++) {
		struct chan *out = &th->fchans[i];
		long type = th_type[i];
		long row = thread->gindex;
		if (prv_register(prv, row, type, &emu->bay, out)) {
			err("prv_register failed");
			return -1;
		}
	}

	return 0;
}

static int
nanos6_connect(struct emu *emu)
{
	struct system *sys = &emu->system;

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
simple(struct emu *emu)
{
	const int *entry = ss_table[emu->ev->c][emu->ev->v];
	int chind = entry[0];
	int action = entry[1];
	int st = entry[2];

	struct nanos6_thread *th = extend_get(&emu->thread->ext, '6');
	struct chan *ch = &th->chans[chind];

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
//		case 'T':
//			pre_task(emu);
//			break;
//		case 'Y':
//			pre_type(emu);
//			break;
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
