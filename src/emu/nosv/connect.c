#include "nosv_priv.h"

static const int th_track[CH_MAX] = {
	[CH_TASKID]    = TRACK_TH_RUN,
	[CH_TYPE]      = TRACK_TH_RUN,
	[CH_APPID]     = TRACK_TH_RUN,
	[CH_SUBSYSTEM] = TRACK_TH_ACT,
	[CH_RANK]      = TRACK_TH_RUN,
};

static const int cpu_track[CH_MAX] = {
	[CH_TASKID]    = TRACK_TH_RUN,
	[CH_TYPE]      = TRACK_TH_RUN,
	[CH_APPID]     = TRACK_TH_RUN,
	[CH_SUBSYSTEM] = TRACK_TH_RUN,
	[CH_RANK]      = TRACK_TH_RUN,
};

int
nosv_get_track(int c, int type)
{
	if (type == CT_TH)
		return th_track[c];
	else
		return cpu_track[c];
}

static int
connect_cpu(struct emu *emu, struct cpu *scpu)
{
	struct nosv_cpu *cpu = EXT(scpu, 'V');
	for (int i = 0; i < CH_MAX; i++) {
		struct track *track = &cpu->track[i];

		/* Choose select CPU channel based on tracking mode (only
		 * TRACK_TH_RUN allowed, as active may cause collisions) */
		int mode = nosv_get_track(i, CT_CPU);
		struct chan *sel = cpu_get_th_chan(scpu, mode);
		if (track_set_select(track, mode, sel, NULL) != 0) {
			err("track_select failed");
			return -1;
		}

		/* Add each thread as input */
		for (struct thread *t = emu->system.threads; t; t = t->gnext) {
			struct nosv_thread *th = EXT(t, 'V');

			/* Choose input channel from the thread output channels
			 * based on CPU tracking mode */
			struct value key = value_int64(t->gindex);
			struct chan *inp = track_get_output(&th->track[i], mode);

			if (track_add_input(track, mode, key, inp) != 0) {
				err("track_add_input failed");
				return -1;
			}
		}

		/* Set the PRV output */
		track_set_default(track, nosv_get_track(i, CT_CPU));
	}

	return 0;
}

int
nosv_connect(struct emu *emu)
{
	struct system *sys = &emu->system;

	/* threads */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		struct nosv_thread *th = EXT(t, 'V');
		struct chan *sel = &t->chan[TH_CHAN_STATE];
		if (track_connect_thread(th->track, th->ch, th_track, sel, CH_MAX) != 0) {
			err("track_thread failed");
			return -1;
		}
	}

	/* cpus */
	for (struct cpu *c = sys->cpus; c; c = c->next) {
		if (connect_cpu(emu, c) != 0) {
			err("connect_cpu failed");
			return -1;
		}
	}

	if (nosv_init_pvt(emu) != 0) {
		err("init_pvt failed");
		return -1;
	}

	return 0;
}
