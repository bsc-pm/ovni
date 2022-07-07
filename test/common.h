/*
 * Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
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

#ifndef OVNI_TEST_COMMON_H
#define OVNI_TEST_COMMON_H

#include "../common.h"
#include "ovni.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define INSTR_0ARG(name, mcv) \
	static inline void name(void) \
	{ \
		struct ovni_ev ev = {0}; \
		ovni_ev_set_clock(&ev, ovni_clock_now()); \
		ovni_ev_set_mcv(&ev, mcv); \
		ovni_ev_emit(&ev); \
	}

#define INSTR_1ARG(name, mcv, ta, a) \
	static inline void name(ta a) \
	{ \
		struct ovni_ev ev = {0}; \
		ovni_ev_set_clock(&ev, ovni_clock_now()); \
		ovni_ev_set_mcv(&ev, mcv); \
		ovni_payload_add(&ev, (uint8_t *) &a, sizeof(a)); \
		ovni_ev_emit(&ev); \
	}

#define INSTR_2ARG(name, mcv, ta, a, tb, b) \
	static inline void name(ta a, tb b) \
	{ \
		struct ovni_ev ev = {0}; \
		ovni_ev_set_clock(&ev, ovni_clock_now()); \
		ovni_ev_set_mcv(&ev, mcv); \
		ovni_payload_add(&ev, (uint8_t *) &a, sizeof(a)); \
		ovni_payload_add(&ev, (uint8_t *) &b, sizeof(b)); \
		ovni_ev_emit(&ev); \
	}

#define INSTR_3ARG(name, mcv, ta, a, tb, b, tc, c) \
	static inline void name(ta a, tb b, tc c) \
	{ \
		struct ovni_ev ev = {0}; \
		ovni_ev_set_mcv(&ev, mcv); \
		ovni_ev_set_clock(&ev, ovni_clock_now()); \
		ovni_payload_add(&ev, (uint8_t *) &a, sizeof(a)); \
		ovni_payload_add(&ev, (uint8_t *) &b, sizeof(b)); \
		ovni_payload_add(&ev, (uint8_t *) &c, sizeof(c)); \
		ovni_ev_emit(&ev); \
	}

INSTR_3ARG(instr_thread_execute, "OHx", int32_t, cpu, int32_t, creator_tid, uint64_t, tag)

static inline void
instr_thread_end(void)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "OHe");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_emit(&ev);

	/* Flush the events to disk before killing the thread */
	ovni_flush();
}

#endif /* OVNI_TEST_COMMON_H */
