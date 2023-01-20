/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_PLAYER_H
#define EMU_PLAYER_H

#include "emu_trace.h"

#include <linux/limits.h>

struct emu_player {
	heap_head_t heap;
	int64_t firstclock;
	int64_t lastclock;
	int64_t deltaclock;
	int first_event;
	struct emu_stream *stream;
};

int emu_player_init(struct emu_player *player, struct emu_trace *trace);
int emu_player_step(struct emu_player *player);
void emu_player_ev(struct emu_player *player, struct ovni_ev *ev);
void emu_player_stream(struct emu_player *player, struct emu_stream *stream);

#endif /* EMU_PLAYER_H */
