#ifndef MARK_H
#define MARK_H

#include "common.h"
#include "chan.h"

struct emu;
struct mark_chan;

struct ovni_mark_emu {
	/* Hash table of types of marks */
	struct mark_type *types;
	long ntypes;
};

struct ovni_mark_thread {
	struct track *track;
	struct chan *channels;
	long nchannels;
};

struct ovni_mark_cpu {
	struct track *track;
};

USE_RET int mark_create(struct emu *emu);
USE_RET int mark_connect(struct emu *emu);
USE_RET int mark_event(struct emu *emu);

#endif /* MARK_H */
