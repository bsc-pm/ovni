#ifndef MARK_H
#define MARK_H

#include "common.h"

struct emu;
struct mark_chan;

struct ovni_mark_emu {
	/* Hash table of channels */
	struct mark_chan *chan;
	int has_marks;
};

USE_RET int mark_create(struct emu *emu);
USE_RET int mark_connect(struct emu *emu);
USE_RET int mark_event(struct emu *emu);

#endif /* MARK_H */
