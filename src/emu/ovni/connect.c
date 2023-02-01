#include "ovni_priv.h"

static const int th_type[] = {
	[CH_FLUSH] = 7,
};

static const int *cpu_type = th_type;


static int
connect_thread_mux(struct emu *emu, struct thread *thread)
{
	struct ovni_thread *th = extend_get(&thread->ext, 'O');
	for (int i = 0; i < CH_MAX; i++) {
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
	}

	return 0;
}

static int
connect_thread_prv(struct emu *emu, struct thread *thread, struct prv *prv)
{
	struct ovni_thread *th = extend_get(&thread->ext, 'O');
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
		struct ovni_thread *th = extend_get(&t->ext, 'O');
		struct chan *inp = &th->ch_run[i];
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
	struct ovni_cpu *cpu = extend_get(&scpu->ext, 'O');
	for (int i = 0; i < CH_MAX; i++) {
		struct mux *mux = &cpu->mux[i];
		struct chan *out = &cpu->ch[i];
		struct chan *sel = &scpu->chan[CPU_CHAN_THRUN];

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
	struct ovni_cpu *cpu = extend_get(&scpu->ext, 'O');
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

int
ovni_connect(struct emu *emu)
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
