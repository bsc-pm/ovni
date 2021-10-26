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

#ifndef OVNI_TRACE_H
#define OVNI_TRACE_H

#include "ovni.h"
#include "emu.h"

int ovni_load_next_event(struct ovni_stream *stream);

int ovni_load_trace(struct ovni_trace *trace, char *tracedir);

int ovni_load_streams(struct ovni_trace *trace);

void ovni_free_streams(struct ovni_trace *trace);

#endif /* OVNI_TRACE_H */
