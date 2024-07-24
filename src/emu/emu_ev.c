/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu_ev.h"
#include "ovni.h"

void
emu_ev(struct emu_ev *ev, const struct ovni_ev *oev,
		int64_t sclock, int64_t dclock)
{
	ev->m = oev->header.model;
	ev->c = oev->header.category;
	ev->v = oev->header.value;
	ev->mcv[3] = '\0';

	ev->rclock = (int64_t) oev->header.clock;
	ev->sclock = sclock;
	ev->dclock = dclock;

	ev->payload_size = (size_t) ovni_payload_size(oev);

	if (ev->payload_size > 0) {
		ev->has_payload = 1;
		ev->payload = &oev->payload;

		if (oev->header.flags & OVNI_EV_JUMBO) {
			ev->is_jumbo = 1;
		}
	} else {
		ev->has_payload = 0;
		ev->payload = NULL;
		ev->is_jumbo = 0;
	}
}
