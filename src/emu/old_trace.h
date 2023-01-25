/* Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef OVNI_TRACE_H
#define OVNI_TRACE_H

#include "emu.h"
#include "ovni.h"

int ovni_load_next_event(struct ovni_stream *stream);

int ovni_load_trace(struct ovni_trace *trace, char *tracedir);

int ovni_load_streams(struct ovni_trace *trace);

void ovni_free_streams(struct ovni_trace *trace);

void ovni_free_trace(struct ovni_trace *trace);

#endif /* OVNI_TRACE_H */
