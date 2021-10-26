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

#ifndef OVNI_PRV_H
#define OVNI_PRV_H

#include <stdint.h>
#include "ovni.h"
#include "emu.h"

void
prv_ev(FILE *f, int row, int64_t time, int type, int val);

void
prv_ev_thread_raw(struct ovni_emu *emu, int row, int64_t time, int type, int val);

void
prv_ev_thread(struct ovni_emu *emu, int row, int type, int val);

void
prv_ev_cpu(struct ovni_emu *emu, int row, int type, int val);

void
prv_ev_autocpu(struct ovni_emu *emu, int type, int val);

void
prv_ev_autocpu_raw(struct ovni_emu *emu, int64_t time, int type, int val);

void
prv_header(FILE *f, int nrows);

#endif /* OVNI_PRV_H */
