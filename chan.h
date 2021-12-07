/*
 * Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef OVNI_CHAN_H
#define OVNI_CHAN_H

#include "emu.h"

void
chan_th_init(struct ovni_ethread *th,
		struct ovni_chan **update_list,
		enum chan id,
		enum chan_track track,
		int init_st,
		int enabled,
		int dirty,
		int row,
		FILE *prv,
		int64_t *clock);

void
chan_cpu_init(struct ovni_cpu *cpu,
		struct ovni_chan **update_list,
		enum chan id,
		enum chan_track track,
		int row,
		int init_st,
		int enabled,
		int dirty,
		FILE *prv,
		int64_t *clock);

void
chan_enable(struct ovni_chan *chan, int enabled);

void
chan_disable(struct ovni_chan *chan);

int
chan_is_enabled(struct ovni_chan *chan);

void
chan_set(struct ovni_chan *chan, int st);

void
chan_push(struct ovni_chan *chan, int st);

int
chan_pop(struct ovni_chan *chan, int expected_st);

void
chan_ev(struct ovni_chan *chan, int ev);

int
chan_get_st(struct ovni_chan *chan);

void
chan_emit(struct ovni_chan *chan);

#endif /* OVNI_CHAN_H */
