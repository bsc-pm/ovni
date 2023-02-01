/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_PLAYER_H
#define EMU_PLAYER_H

#include "trace.h"
#include "emu_ev.h"

#include <linux/limits.h>

struct emu_player {
	heap_head_t heap;
	int64_t firstclock;
	int64_t lastclock;
	int64_t deltaclock;
	int first_event;
	struct stream *stream;
	struct emu_ev ev;
};

int emu_player_init(struct emu_player *player, struct trace *trace);
int emu_player_step(struct emu_player *player);
struct emu_ev *emu_player_ev(struct emu_player *player);
struct stream *emu_player_stream(struct emu_player *player);

#endif /* EMU_PLAYER_H */
