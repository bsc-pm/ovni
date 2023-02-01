#include "nanos6_priv.h"

const enum nanos6_track th_track[] = {
	[CH_TASKID]    = RUN_TH,
	[CH_TYPE]      = RUN_TH,
	[CH_SUBSYSTEM] = ACT_TH,
	[CH_RANK]      = RUN_TH,
	[CH_THREAD]    = NONE,
};

const enum nanos6_track cpu_track[] = {
	[CH_TASKID]    = RUN_TH,
	[CH_TYPE]      = RUN_TH,
	[CH_SUBSYSTEM] = RUN_TH,
	[CH_RANK]      = RUN_TH,
	[CH_THREAD]    = RUN_TH,
};

static const int th_type[] = {
	[CH_TASKID]    = 35,
	[CH_TYPE]      = 36,
	[CH_SUBSYSTEM] = 37,
	[CH_RANK]      = 38,
	[CH_THREAD]    = 39,
};

static const int *cpu_type = th_type;


static int
connect_thread_mux(struct emu *emu, struct thread *thread)
{
	struct nanos6_thread *th = EXT(thread, '6');
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
		if (th_track[i] == RUN_TH)
			th->ch_out[i] = &th->ch_run[i];
		else if (th_track[i] == ACT_TH)
			th->ch_out[i] = &th->ch_act[i];
		else
			th->ch_out[i] = &th->ch[i];

	}

	return 0;
}

static int
connect_thread_prv(struct emu *emu, struct thread *thread, struct prv *prv)
{
	struct nanos6_thread *th = EXT(thread, '6');
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
		struct nanos6_thread *th = EXT(t, '6');

		/* Choose input thread channel based on tracking mode */
		struct chan *inp = NULL;
		if (cpu_track[i] == RUN_TH)
			inp = &th->ch_run[i];
		else if (cpu_track[i] == ACT_TH)
			inp = &th->ch_act[i];
		else
			die("cpu tracking must be 'running' or 'active'");

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
	struct nanos6_cpu *cpu = EXT(scpu, '6');
	for (int i = 0; i < CH_MAX; i++) {
		struct mux *mux = &cpu->mux[i];
		struct chan *out = &cpu->ch[i];

		/* Choose select CPU channel based on tracking mode */
		struct chan *sel = NULL;
		if (cpu_track[i] == RUN_TH)
			sel = &scpu->chan[CPU_CHAN_THRUN];
		else if (cpu_track[i] == ACT_TH)
			sel = &scpu->chan[CPU_CHAN_THACT];
		else
			die("cpu tracking must be 'running' or 'active'");

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
	struct nanos6_cpu *cpu = EXT(scpu, '6');
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

//static int
//populate_cpu_pcf(struct emu *emu, struct pcf *pcf)
//{
//}

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

//	struct pcf *pcf = pvt_get_pcf(pvt);
//	populate_cpu_pcf(pcf, emu);

	return 0;
}

int
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
