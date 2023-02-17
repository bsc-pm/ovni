#include "nodes_priv.h"

static const char model_name[] = "nodes";
enum { model_id = 'D' };

struct model_spec model_nodes = {
	.name = model_name,
	.model = model_id,
	.create  = nodes_create,
//	.connect = nodes_connect,
	.event   = nodes_event,
	.probe   = nodes_probe,
	.finish  = nodes_finish,
};

/* ----------------- channels ------------------ */

static const char *chan_name[CH_MAX] = {
	[CH_SUBSYSTEM] = "subsystem",
};

static const int chan_stack[CH_MAX] = {
	[CH_SUBSYSTEM] = 1,
};

/* ----------------- pvt ------------------ */

static const int pvt_type[] = {
	[CH_SUBSYSTEM] = 30,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_SUBSYSTEM]   = "NODES subsystem",
};

static const struct pcf_value_label nodes_ss_values[] = {
	{ ST_REGISTER,    "Dependencies: Registering task accesses" },
	{ ST_UNREGISTER,  "Dependencies: Unregistering task accesses" },
	{ ST_IF0_WAIT,    "If0: Waiting for an If0 task" },
	{ ST_IF0_INLINE,  "If0: Executing an If0 task inline" },
	{ ST_TASKWAIT,    "Taskwait: Taskwait" },
	{ ST_CREATE,      "Add Task: Creating a task" },
	{ ST_SUBMIT,      "Add Task: Submitting a task" },
	{ ST_SPAWN,       "Spawn Function: Spawning a function" },
	{ -1, NULL },
};

static const struct pcf_value_label (*pcf_labels[CH_MAX])[] = {
	[CH_SUBSYSTEM] = &nodes_ss_values,
};

static const struct model_pvt_spec pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
};

/* ----------------- tracking ------------------ */

static const int th_track[CH_MAX] = {
	[CH_SUBSYSTEM] = TRACK_TH_RUN, /* FIXME: Why not active? */
};

static const int cpu_track[CH_MAX] = {
	[CH_SUBSYSTEM] = TRACK_TH_RUN,
};

/* ----------------- chan_spec ------------------ */

static const struct model_chan_spec th_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.pvt = &pvt_spec,
	.track = th_track,
};

static const struct model_chan_spec cpu_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.pvt = &pvt_spec,
	.track = cpu_track,
};

/* ----------------- models ------------------ */

static const struct model_cpu_spec cpu_spec = {
	.size = sizeof(struct nodes_cpu),
	.chan = &cpu_chan,
	.model = &model_nodes,
};

static const struct model_thread_spec th_spec = {
	.size = sizeof(struct nodes_thread),
	.chan = &th_chan,
	.model = &model_nodes,
};

/* ----------------------------------------------------- */

int
nodes_probe(struct emu *emu)
{
	if (emu->system.nthreads == 0)
		return 1;

	return 0;
}

int
nodes_create(struct emu *emu)
{
	if (model_thread_create(emu, &th_spec) != 0) {
		err("model_thread_init failed");
		return -1;
	}

	if (model_cpu_create(emu, &cpu_spec) != 0) {
		err("model_cpu_init failed");
		return -1;
	}

	return 0;
}

int
nodes_connect(struct emu *emu)
{
	if (model_thread_connect(emu, &th_spec) != 0) {
		err("model_thread_connect failed");
		return -1;
	}

	if (model_cpu_connect(emu, &cpu_spec) != 0) {
		err("model_cpu_connect failed");
		return -1;
	}

	return 0;
}

static int
end_lint(struct emu *emu)
{
	struct system *sys = &emu->system;

	/* Ensure we run out of subsystem states */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		struct nodes_thread *th = EXT(t, model_id);
		struct chan *ch = &th->m.ch[CH_SUBSYSTEM];
		int stacked = ch->data.stack.n;
		if (stacked > 0) {
			struct value top;
			if (chan_read(ch, &top) != 0) {
				err("chan_read failed for subsystem");
				return -1;
			}

			err("thread %d ended with %d stacked nodes subsystems",
					t->tid, stacked);
			return -1;
		}
	}

	return 0;
}

int
nodes_finish(struct emu *emu)
{
	/* When running in linter mode perform additional checks */
	if (emu->args.linter_mode && end_lint(emu) != 0) {
		err("end_lint failed");
		return -1;
	}

	return 0;
}
