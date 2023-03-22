/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_PLAYER_H
#define EMU_PLAYER_H

#include <stdint.h>
#include "common.h"
#include "emu_ev.h"
#include "heap.h"
struct trace;

struct player {
	struct trace *trace;
	heap_head_t heap;
	int64_t firstclock;
	int64_t lastclock;
	int64_t deltaclock;
	int64_t nprocessed;
	int first_event;
	int unsorted;
	struct stream *stream;
	struct emu_ev ev;
};

USE_RET int player_init(struct player *player, struct trace *trace, int unsorted);
USE_RET int player_step(struct player *player);
USE_RET struct emu_ev *player_ev(struct player *player);
USE_RET struct stream *player_stream(struct player *player);
USE_RET double player_progress(struct player *player);
USE_RET int64_t player_nprocessed(struct player *player);

#endif /* EMU_PLAYER_H */
