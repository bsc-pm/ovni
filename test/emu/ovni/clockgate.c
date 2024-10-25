/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "compat.h"
#include "ovni.h"
#include "instr.h"

int64_t delta = 0LL;

static void
thread_execute_delayed(int32_t cpu, int32_t creator_tid, uint64_t tag)
{
	struct ovni_ev ev = {0};
	ovni_ev_set_mcv(&ev, "OHx");
	ovni_ev_set_clock(&ev, ovni_clock_now() + (uint64_t) delta);
	ovni_payload_add(&ev, (uint8_t *) &cpu, sizeof(cpu));
	ovni_payload_add(&ev, (uint8_t *) &creator_tid, sizeof(creator_tid));
	ovni_payload_add(&ev, (uint8_t *) &tag, sizeof(tag));
	ovni_ev_emit(&ev);
}

static inline void
start_delayed(int rank, int nranks)
{
	char hostname[OVNI_MAX_HOSTNAME];
	char rankname[OVNI_MAX_HOSTNAME + 64];

	if (gethostname(hostname, OVNI_MAX_HOSTNAME) != 0)
		die("gethostname failed");

	sprintf(rankname, "%s.%d", hostname, rank);

	ovni_version_check();
	ovni_proc_init(1, rankname, getpid());
	ovni_thread_init(get_tid());
	ovni_proc_set_rank(rank, nranks);

	/* All ranks inform CPUs */
	for (int i = 0; i < nranks; i++)
		ovni_add_cpu(i, i);

	int curcpu = rank;

	dbg("thread %d has cpu %d (ncpus=%d)",
			get_tid(), curcpu, nranks);

	delta = ((int64_t) rank) * (int64_t) (2LL * 3600LL * 1000LL * 1000LL * 1000LL);
	thread_execute_delayed(curcpu, -1, 0);
}

int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));

	start_delayed(rank, nranks);

	ovni_flush();
	ovni_thread_free();
	ovni_proc_fini();

	return 0;
}
