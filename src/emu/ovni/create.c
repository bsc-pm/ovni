#include "ovni_priv.h"

static const char chan_fmt_cpu[] = "ovni.cpu%ld.%s";
static const char chan_fmt_th_raw[] = "ovni.thread%ld.%s.raw";
static const char chan_fmt_th_run[] = "ovni.thread%ld.%s.run";

static const char *chan_name[] = {
	[CH_FLUSH] = "flush",
};

static int
init_chans(struct bay *bay, struct chan *chans, const char *fmt, int64_t gindex)
{
	for (int i = 0; i < CH_MAX; i++) {
		struct chan *c = &chans[i];
		chan_init(c, CHAN_SINGLE, fmt, gindex, chan_name[i]);

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
	struct ovni_cpu *cpu = calloc(1, sizeof(struct ovni_cpu));
	if (cpu == NULL) {
		err("calloc failed:");
		return -1;
	}

	if (init_chans(bay, cpu->ch, chan_fmt_cpu, syscpu->gindex) != 0) {
		err("init_chans failed");
		return -1;
	}

	extend_set(&syscpu->ext, 'O', cpu);
	return 0;
}

static int
init_thread(struct bay *bay, struct thread *systh)
{
	struct ovni_thread *th = calloc(1, sizeof(struct ovni_thread));
	if (th == NULL) {
		err("calloc failed:");
		return -1;
	}

	if (init_chans(bay, th->ch, chan_fmt_th_raw, systh->gindex) != 0) {
		err("init_chans failed");
		return -1;
	}

	if (init_chans(bay, th->ch_run, chan_fmt_th_run, systh->gindex) != 0) {
		err("init_chans failed");
		return -1;
	}

	extend_set(&systh->ext, 'O', th);

	return 0;
}

int
ovni_create(struct emu *emu)
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

	return 0;
}
