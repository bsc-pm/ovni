#include "nosv_priv.h"

const enum nosv_track nosv_chan_track[CH_MAX][CT_MAX] = {
	                /* Thread  CPU */
	[CH_TASKID]    = { RUN_TH, RUN_TH },
	[CH_TYPE]      = { RUN_TH, RUN_TH },
	[CH_APPID]     = { RUN_TH, RUN_TH },
	[CH_SUBSYSTEM] = { ACT_TH, RUN_TH },
	[CH_RANK]      = { RUN_TH, RUN_TH },
};


static int
connect_thread_mux(struct emu *emu, struct thread *thread)
{
	struct nosv_thread *th = EXT(thread, 'V');
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
		enum nosv_track track = nosv_chan_track[i][CT_TH];
		if (track == RUN_TH)
			th->ch_out[i] = &th->ch_run[i];
		else if (track == ACT_TH)
			th->ch_out[i] = &th->ch_act[i];
		else
			th->ch_out[i] = &th->ch[i];

	}

	return 0;
}

static int
add_inputs_cpu_mux(struct emu *emu, struct mux *mux, int i)
{
	for (struct thread *t = emu->system.threads; t; t = t->gnext) {
		struct nosv_thread *th = EXT(t, 'V');

		/* Choose input thread channel based on tracking mode */
		struct chan *inp = NULL;
		enum nosv_track track = nosv_chan_track[i][CT_CPU];
		if (track == RUN_TH)
			inp = &th->ch_run[i];
		else if (track == ACT_TH)
			inp = &th->ch_act[i];
		else
			die("cpu tracking must be running or active");

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
	struct nosv_cpu *cpu = EXT(scpu, 'V');
	for (int i = 0; i < CH_MAX; i++) {
		struct mux *mux = &cpu->mux[i];
		struct chan *out = &cpu->ch[i];

		/* Choose select CPU channel based on tracking mode */
		struct chan *sel = NULL;
		enum nosv_track track = nosv_chan_track[i][CT_CPU];
		if (track == RUN_TH)
			sel = &scpu->chan[CPU_CHAN_THRUN];
		else if (track == ACT_TH)
			sel = &scpu->chan[CPU_CHAN_THACT];
		else
			die("cpu tracking must be running or active");

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

int
nosv_connect(struct emu *emu)
{
	struct system *sys = &emu->system;

	/* threads */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		if (connect_thread_mux(emu, t) != 0) {
			err("connect_thread_mux failed");
			return -1;
		}
	}

	/* cpus */
	for (struct cpu *c = sys->cpus; c; c = c->next) {
		if (connect_cpu_mux(emu, c) != 0) {
			err("connect_cpu_mux failed");
			return -1;
		}
	}

	if (nosv_init_pvt(emu) != 0) {
		err("init_pvt failed");
		return -1;
	}

	return 0;
}
