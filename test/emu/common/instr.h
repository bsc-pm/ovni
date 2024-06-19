/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef INSTR_H
#define INSTR_H

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "common.h"
#include "compat.h"
#include "ovni.h"
#include "emu/models.h"

extern int first_clock_set;
extern int64_t first_clock;
extern int64_t last_clock;

int64_t get_clock(void);
int64_t get_delta(void);

#define INSTR_0ARG(name, mcv)                             \
	static inline void name(void)                     \
	{                                                 \
		struct ovni_ev ev = {0};                  \
		ovni_ev_set_clock(&ev, get_clock());      \
		ovni_ev_set_mcv(&ev, mcv);                \
		ovni_ev_emit(&ev);                        \
	}

#define INSTR_1ARG(name, mcv, ta, a)                              \
	static inline void name(ta a)                             \
	{                                                         \
		struct ovni_ev ev = {0};                          \
		ovni_ev_set_clock(&ev, get_clock());              \
		ovni_ev_set_mcv(&ev, mcv);                        \
		ovni_payload_add(&ev, (uint8_t *) &a, sizeof(a)); \
		ovni_ev_emit(&ev);                                \
	}

#define INSTR_2ARG(name, mcv, ta, a, tb, b)                       \
	static inline void name(ta a, tb b)                       \
	{                                                         \
		struct ovni_ev ev = {0};                          \
		ovni_ev_set_clock(&ev, get_clock());              \
		ovni_ev_set_mcv(&ev, mcv);                        \
		ovni_payload_add(&ev, (uint8_t *) &a, sizeof(a)); \
		ovni_payload_add(&ev, (uint8_t *) &b, sizeof(b)); \
		ovni_ev_emit(&ev);                                \
	}

#define INSTR_3ARG(name, mcv, ta, a, tb, b, tc, c)                \
	static inline void name(ta a, tb b, tc c)                 \
	{                                                         \
		struct ovni_ev ev = {0};                          \
		ovni_ev_set_clock(&ev, get_clock());              \
		ovni_ev_set_mcv(&ev, mcv);                        \
		ovni_payload_add(&ev, (uint8_t *) &a, sizeof(a)); \
		ovni_payload_add(&ev, (uint8_t *) &b, sizeof(b)); \
		ovni_payload_add(&ev, (uint8_t *) &c, sizeof(c)); \
		ovni_ev_emit(&ev);                                \
	}

INSTR_3ARG(instr_thread_execute, "OHx", int32_t, cpu, int32_t, creator_tid, uint64_t, tag)
INSTR_0ARG(instr_thread_pause, "OHp")
INSTR_0ARG(instr_thread_resume, "OHr")
INSTR_1ARG(instr_thread_affinity_set, "OAs", int32_t, cpu)

static inline void
instr_thread_end(void)
{
	struct ovni_ev ev = {0};

	ovni_ev_set_mcv(&ev, "OHe");
	ovni_ev_set_clock(&ev, get_clock());
	ovni_ev_emit(&ev);

	/* Flush the events to disk before killing the thread */
	ovni_flush();
}

static inline void
instr_require(const char *model)
{
	const char *version = models_get_version(model);
	if (version == NULL)
		die("cannot find version of model %s", model);

	ovni_thread_require(model, version);
}

static inline void
instr_start(int rank, int nranks)
{
	char hostname[OVNI_MAX_HOSTNAME];
	char rankname[OVNI_MAX_HOSTNAME + 64];

	if (gethostname(hostname, OVNI_MAX_HOSTNAME) != 0)
		die("gethostname failed");

	sprintf(rankname, "%s.%d", hostname, rank);

	ovni_version_check();
	ovni_proc_init(1, rankname, getpid());
	ovni_proc_set_rank(rank, nranks);
	ovni_thread_init(get_tid());

	/* All ranks inform CPUs */
	for (int i = 0; i < nranks; i++)
		ovni_add_cpu(i, i);

	int curcpu = rank;

	dbg("thread %d has cpu %d (ncpus=%d)",
			get_tid(), curcpu, nranks);

	instr_require("ovni");
	instr_thread_execute(curcpu, -1, 0);
}

static inline void
instr_end(void)
{
	instr_thread_end();
	ovni_thread_free();
	ovni_proc_fini();
}

#endif /* INSTR_H */
