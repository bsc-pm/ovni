#include "nodes_priv.h"

static int
end_lint(struct emu *emu)
{
	struct system *sys = &emu->system;

	/* Ensure we run out of subsystem states */
	for (struct thread *t = sys->threads; t; t = t->gnext) {
		struct nodes_thread *th = EXT(t, 'D');
		struct chan *ch = &th->ch[CH_SUBSYSTEM];
		int stacked = ch->data.stack.n;
		if (stacked > 0) {
			struct value top;
			if (chan_read(ch, &top) != 0) {
				err("chan_read failed for subsystem");
				return -1;
			}

			err("thread %d ended with %d stacked nodes subsystems, top=\"%s\"\n",
					t->tid, stacked, nodes_ss_name(top.i));
			return -1;
		}
	}

	return 0;
}

int
nodes_finish(struct emu *emu)
{
	if (nodes_finish_pvt(emu) != 0) {
		err("finish_pvt failed");
		return -1;
	}

	/* When running in linter mode perform additional checks */
	if (emu->args.linter_mode && end_lint(emu) != 0) {
		err("end_lint failed");
		return -1;
	}

	return 0;
}
