/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "instr.h"
#include "ovni.h"

static void
emit(uint8_t *buf, size_t size)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, "OB.");
	ovni_ev_set_clock(&ev, ovni_clock_now());
	ovni_ev_jumbo_emit(&ev, buf, (uint32_t) size);
}

int
main(void)
{
	instr_start(0, 1);

	size_t payload_size = (size_t) (0.9 * (double) OVNI_MAX_EV_BUF);
	uint8_t *payload_buf = calloc(1, payload_size);

	if (!payload_buf) {
		perror("calloc failed");
		exit(EXIT_FAILURE);
	}

	/* The first should fit, filling 90 % of the buffer */
	emit(payload_buf, payload_size);

	/* The second event would cause a flush */
	emit(payload_buf, payload_size);

	/* The third would cause the previous flush events to be written
	 * to disk */
	emit(payload_buf, payload_size);

	/* Flush the last event to disk manually */
	instr_end();

	free(payload_buf);

	return 0;
}
