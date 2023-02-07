#include "nosv_priv.h"

static const char chan_fmt_cpu_raw[] = "nosv.cpu%ld.%s";
//static const char chan_fmt_cpu_run[] = "nosv.cpu%ld.%s.run";
//static const char chan_fmt_cpu_act[] = "nosv.cpu%ld.%s.act";
static const char chan_fmt_th_raw[] = "nosv.thread%ld.%s.raw";
static const char chan_fmt_th_run[] = "nosv.thread%ld.%s.run";
static const char chan_fmt_th_act[] = "nosv.thread%ld.%s.act";

static const char *chan_name[CH_MAX] = {
	[CH_TASKID]    = "taskid",
	[CH_TYPE]      = "task_type",
	[CH_APPID]     = "appid",
	[CH_SUBSYSTEM] = "subsystem",
	[CH_RANK]      = "rank",
};

static const int chan_stack[CH_MAX] = {
	[CH_SUBSYSTEM] = 1,
};

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
	struct nosv_cpu *cpu = calloc(1, sizeof(struct nosv_cpu));
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

	extend_set(&syscpu->ext, 'V', cpu);
	return 0;
}

static int
init_thread(struct bay *bay, struct thread *systh)
{
	struct nosv_thread *th = calloc(1, sizeof(struct nosv_thread));
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

	extend_set(&systh->ext, 'V', th);

	return 0;
}

static int
init_proc(struct proc *sysproc)
{
	struct nosv_proc *proc = calloc(1, sizeof(struct nosv_proc));
	if (proc == NULL) {
		err("calloc failed:");
		return -1;
	}

	extend_set(&sysproc->ext, 'V', proc);

	return 0;
}

int
nosv_create(struct emu *emu)
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
