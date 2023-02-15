#include "nodes_priv.h"

enum { PUSH = 1, POP = 2, IGN = 3 };

#define CHSS CH_SUBSYSTEM

static const int ss_table[256][256][3] = {
	['R'] = {
		['['] = { CHSS, PUSH, ST_REGISTER },
		[']'] = { CHSS, POP,  ST_REGISTER },
	},
	['U'] = {
		['['] = { CHSS, PUSH, ST_UNREGISTER },
		[']'] = { CHSS, POP,  ST_UNREGISTER },
	},
	['W'] = {
		['['] = { CHSS, PUSH, ST_IF0_WAIT },
		[']'] = { CHSS, POP,  ST_IF0_WAIT },
	},
	['I'] = {
		['['] = { CHSS, PUSH, ST_IF0_INLINE },
		[']'] = { CHSS, POP,  ST_IF0_INLINE },
	},
	['T'] = {
		['['] = { CHSS, PUSH, ST_TASKWAIT },
		[']'] = { CHSS, POP,  ST_TASKWAIT },
	},
	['C'] = {
		['['] = { CHSS, PUSH, ST_CREATE },
		[']'] = { CHSS, POP,  ST_CREATE },
	},
	['S'] = {
		['['] = { CHSS, PUSH, ST_SUBMIT },
		[']'] = { CHSS, POP,  ST_SUBMIT },
	},
	['P'] = {
		['['] = { CHSS, PUSH, ST_SPAWN },
		[']'] = { CHSS, POP,  ST_SPAWN },
	},
};

static int
simple(struct emu *emu)
{
	const int *entry = ss_table[emu->ev->c][emu->ev->v];
	int chind = entry[0];
	int action = entry[1];
	int st = entry[2];

	struct nodes_thread *th = EXT(emu->thread, 'D');
	struct chan *ch = &th->m.ch[chind];

	if (action == PUSH) {
		return chan_push(ch, value_int64(st));
	} else if (action == POP) {
		return chan_pop(ch, value_int64(st));
	} else if (action == IGN) {
		return 0; /* do nothing */
	} else {
		err("unknown NODES subsystem event");
		return -1;
	}

	return 0;
}

static int
process_ev(struct emu *emu)
{
	if (!emu->thread->is_running) {
		err("current thread %d not running", emu->thread->tid);
		return -1;
	}

	switch (emu->ev->c) {
		case 'R':
		case 'U':
		case 'W':
		case 'I':
		case 'T':
		case 'C':
		case 'S':
		case 'P':
			return simple(emu);
		default:
			err("unknown NODES event category");
			return -1;
	}

	/* Not reached */
	return 0;
}

int
nodes_event(struct emu *emu)
{
	dbg("in nodes_event");
	if (emu->ev->m != 'D') {
		err("unexpected event model %c\n", emu->ev->m);
		return -1;
	}

	dbg("got nodes event %s", emu->ev->mcv);
	if (process_ev(emu) != 0) {
		err("error processing NODES event");
		return -1;
	}

	return 0;
}
