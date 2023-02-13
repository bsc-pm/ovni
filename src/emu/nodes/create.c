#include "nodes_priv.h"

static const char *chan_name[CH_MAX] = {
	[CH_SUBSYSTEM] = "subsystem",
};

static const int chan_stack[CH_MAX] = {
	[CH_SUBSYSTEM] = 1,
};

static int
init_chans(struct bay *bay, struct chan *chans, const char *fmt, int64_t gindex)
{
	for (int i = 0; i < CH_MAX; i++) {
		struct chan *c = &chans[i];
		int type = chan_stack[i];
		chan_init(c, type, fmt, gindex, chan_name[i]);

		if (bay_register(bay, c) != 0) {
			err("bay_register failed");
			return -1;
		}
	}

	return 0;
}

static int
init_tracks(struct bay *bay, struct track *tracks, const char *fmt, int64_t gindex)
{
	for (int i = 0; i < CH_MAX; i++) {
		struct track *track = &tracks[i];

		if (track_init(track, bay, TRACK_TYPE_TH, fmt, gindex, chan_name[i]) != 0) {
			err("track_init failed");
			return -1;
		}
	}

	return 0;
}

static int
init_cpu(struct bay *bay, struct cpu *syscpu)
{
	struct nodes_cpu *cpu = calloc(1, sizeof(struct nodes_cpu));
	if (cpu == NULL) {
		err("calloc failed:");
		return -1;
	}

	cpu->track = calloc(CH_MAX, sizeof(struct track));
	if (cpu->track == NULL) {
		err("calloc failed:");
		return -1;
	}

	char *fmt = "nodes.cpu%ld.%s";
	if (init_tracks(bay, cpu->track, fmt, syscpu->gindex) != 0) {
		err("init_chans failed");
		return -1;
	}

	extend_set(&syscpu->ext, 'D', cpu);
	return 0;
}

static int
init_thread(struct bay *bay, struct thread *systh)
{
	struct nodes_thread *th = calloc(1, sizeof(struct nodes_thread));
	if (th == NULL) {
		err("calloc failed:");
		return -1;
	}

	th->ch = calloc(CH_MAX, sizeof(struct chan));
	if (th->ch == NULL) {
		err("calloc failed:");
		return -1;
	}

	th->track = calloc(CH_MAX, sizeof(struct track));
	if (th->track == NULL) {
		err("calloc failed:");
		return -1;
	}

	char *fmt = "nodes.thread%ld.%s";
	if (init_chans(bay, th->ch, fmt, systh->gindex) != 0) {
		err("init_chans failed");
		return -1;
	}

	if (init_tracks(bay, th->track, fmt, systh->gindex) != 0) {
		err("init_tracks failed");
		return -1;
	}

	extend_set(&systh->ext, 'D', th);

	return 0;
}

int
nodes_create(struct emu *emu)
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
