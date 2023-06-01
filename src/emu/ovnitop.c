/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "emu_ev.h"
#include "emu_stat.h"
#include "ovni.h"
#include "player.h"
#include "trace.h"
#include "uthash.h"

struct entry {
	char mcv[4];
	long count;
	UT_hash_handle hh;
};

char *tracedir;
struct entry *table = NULL;

static void
accum(struct player *player)
{
	struct emu_ev *ev = player_ev(player);
	struct entry *e = NULL;
	HASH_FIND_STR(table, ev->mcv, e);

	if (e == NULL) {
		e = calloc(1, sizeof(struct entry));
		if (e == NULL)
			die("calloc failed:");

		strcpy(e->mcv, ev->mcv);
		e->count = 1;
		HASH_ADD_STR(table, mcv, e);
	} else {
		e->count++;
	}
}

static int
by_count(struct entry *a, struct entry *b)
{
	if (a->count < b->count)
		return +1;
	if (a->count > b->count)
		return -1;
	
	/* Otherwise they have the same count, sort by mcv */
	return strcmp(a->mcv, b->mcv);
}

static void
report(void)
{
	HASH_SORT(table, by_count);

	for (struct entry *e = table; e; e = e->hh.next)
		printf("%s %10ld\n", e->mcv, e->count);

	struct entry *e, *tmp;
	HASH_ITER(hh, table, e, tmp) {
		HASH_DEL(table, e);
		free(e);
	}
}

static void
usage(void)
{
	rerr("Usage: ovnitop DIR\n");
	rerr("\n");
	rerr("Show most common events in a trace.\n");
	rerr("\n");
	rerr("  DIR      Directory containing ovni traces (%s) or single stream.\n",
			OVNI_STREAM_EXT);
	rerr("\n");

	exit(EXIT_FAILURE);
}

static void
parse_args(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "h")) != -1) {
		switch (opt) {
			case 'h':
			default: /* '?' */
				usage();
		}
	}

	if (optind >= argc) {
		err("bad usage: missing directory");
		usage();
	}

	tracedir = argv[optind];
}

int
main(int argc, char *argv[])
{
	progname_set("ovnitop");

	parse_args(argc, argv);

	struct trace *trace = calloc(1, sizeof(struct trace));

	if (trace == NULL) {
		err("calloc failed:");
		return 1;
	}

	if (trace_load(trace, tracedir) != 0) {
		err("failed to load trace: %s", tracedir);
		return 1;
	}

	struct player *player = calloc(1, sizeof(struct player));
	if (player == NULL) {
		err("calloc failed:");
		return 1;
	}

	if (player_init(player, trace, 1) != 0) {
		err("player_init failed");
		return 1;
	}

	int ret;

	struct emu_stat stat;

	emu_stat_init(&stat);

	while ((ret = player_step(player)) == 0) {
		accum(player);
		emu_stat_update(&stat, player);
	}

	emu_stat_report(&stat, player, 1);

	/* Report events */
	report();

	/* Error happened */
	if (ret < 0) {
		err("player_step failed");
		return 1;
	}

	free(trace);
	free(player);

	return 0;
}
