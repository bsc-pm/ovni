/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stddef.h>
#include <stdint.h>
#include "compat.h"
#include "instr.h"
#include "ovni.h"

static void
emit_jumbo(uint8_t *buf, size_t size, int64_t clock)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, "OUj");
	ovni_ev_set_clock(&ev, (uint64_t) clock);
	ovni_ev_jumbo_emit(&ev, buf, (uint32_t) size);
}

static void
emit(char *mcv, int64_t clock)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, mcv);
	ovni_ev_set_clock(&ev, (uint64_t) clock);
	ovni_ev_emit(&ev);
}

static void
fill(long room)
{
	/* Flush the buffer so we begin with a known size: the two flush
	 * events */
	ovni_flush();

	size_t ev_size = sizeof(struct ovni_ev_header);

	/* Skip the jumbo event header and payload size */
	size_t header = ev_size + 4;

	size_t payload_size = (size_t) OVNI_MAX_EV_BUF - (size_t) room - header;
	uint8_t *payload_buf = calloc(1, payload_size);

	/* Fill the stream buffer */
	int64_t t = (int64_t) ovni_clock_now();
	emit_jumbo(payload_buf, payload_size, t);

	/* Leave some room to prevent clashes */
	sleep_us(100);
}

static void
test_flush_after_sort(void)
{
	/* In this test we check with the flush events OF[ and OF]
	 * immediately after the sort region opening event OU[.
	 *
	 * $ ovnidump thread.150439.obs | cut -c -72
	 * ovnidump: INFO: loaded 1 streams
	 *   1778730985500133  +1778730985500133  OHx :00:00:00:00:ff:ff:ff:ff:00:0
	 *   1778730985503455       +3322  OF[
	 *   1778730985508228       +4773  OF]
	 *   1778730985666648     +158420  OUj :cc:ff:1f:00:00:00:00:00:00:00:00:00
	 *   1778730985666649          +1  OU[
	 *   1778730987234771    +1568122  OF[  <-- here we inject the
	 *   1778730988411065    +1176294  OF]      flush events
	 *   1778730985666550    -2744515  KCI
	 *   1778730985666551          +1  KCO
	 *   1778730985666652        +101  OU]
	 *   1778730988411997    +2745345  OHe
	 */

	/* Skip the two flush events and leave room for OU[ */
	fill(3 * (long) sizeof(struct ovni_ev_header));

	int64_t t = (int64_t) ovni_clock_now();

	/* Emit the opening of the sort region */
	emit("OU[", t++);

	/* This should cause the stream to be flush and inject the two
	 * extra events OF[ and OF] */
	emit("KCO", t++ - 100);

	/* Finish the kernel event */
	emit("KCI", t++ - 100);

	/* Finish the sort region */
	emit("OU]", (int64_t) ovni_clock_now());
}

static void
test_unsorted(void)
{
	/* Test unsorted events in the sorting region */

	sleep_us(100); /* Make room */
	int64_t t = (int64_t) ovni_clock_now();
	emit("OU[", t);
	emit("KCI", t + 2 - 100); /* out of order */
	emit("KCO", t + 1 - 100);
	emit("OU]", t + 3);
}

static void
test_overlap(void)
{
	/* Test overlapping events among regions */

	int64_t t = (int64_t) ovni_clock_now();
	/* Round time next 1 microsecond to be easier to read */
	t += 1000 - (t % 1000);

	emit("OU[", t + 20);
	emit("KCO", t + 10);
	emit("KCI", t + 11);
	emit("OU]", t + 21);
	emit("OU[", t + 22);
	emit("KCO", t + 12); /* These two shold appear in the first region */
	emit("KCI", t + 13);
	emit("OU]", t + 23);
}

static void
test_overlap_flush(void)
{
	/* Test overlapping events with flush events too */

	/* Skip the two flush events and leave room for OU[ */
	sleep_us(100);
	fill(5 * (long) sizeof(struct ovni_ev_header));
	int64_t t = (int64_t) ovni_clock_now();

	/* Round time next 1 microsecond to be easier to read */
	t += 1000 - (t % 1000);

	emit("OU[", t + 20);
	emit("KCO", t + 10);
	emit("KCI", t + 11);
	emit("OU]", t + 21);
	/* We need realistic clock due to incoming flush */
	emit("OU[", (int64_t) ovni_clock_now());
	/* Flush here */
	emit("KCO", t + 12);
	emit("KCI", t + 13);
	emit("OU]", (int64_t) ovni_clock_now());
}

int
main(void)
{
	/* This program tests various cases in which flush events are placed in
	 * the sorting region, disrupting the order. */

	instr_start(0, 1);
	instr_require("kernel");
	test_flush_after_sort();
	test_unsorted();
	test_overlap();
	test_overlap_flush();
	instr_end();

	return 0;
}
