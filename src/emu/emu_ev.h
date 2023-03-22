/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef EMU_EV_H
#define EMU_EV_H

#include <stddef.h>
#include <stdint.h>
struct ovni_ev;

/* Easier to parse emulation event */
struct emu_ev {
	union {
		struct {
			uint8_t m;
			uint8_t c;
			uint8_t v;
			uint8_t nil;
		};
		char mcv[4];
	};

	int64_t rclock; /* As-is clock in the binary stream */
	int64_t sclock; /* Corrected clock with stream offset */
	int64_t dclock; /* Delta corrected clock with stream offset */

	int has_payload;
	size_t payload_size;
	int is_jumbo;
	const union ovni_ev_payload *payload; /* NULL if no payload */
};

void emu_ev(struct emu_ev *ev, const struct ovni_ev *oev, int64_t sclock, int64_t dclock);

#endif /* EMU_EV_H */
