/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "emu_ev.h"
#include "model.h"
#include "models.h"
#include "ovni.h"
#include "player.h"
#include "stream.h"
#include "trace.h"

char *tracedir;
int hex_mode = 0;

static void
emit(struct model *model, struct player *player)
{
	struct emu_ev *ev = player_ev(player);
	struct stream *stream = player_stream(player);

	printf("%10" PRIi64 "  %c%c%c  %s  ",
			ev->rclock,
			ev->m,
			ev->c,
			ev->v,
			stream->relpath);

	if (hex_mode) {
		if (ev->has_payload) {
			for (size_t i = 0; i < ev->payload_size; i++)
				printf(":%02x", ev->payload->u8[i]);
		}
	} else {
		char buf[1024];
		if (model_event_print(model, ev, buf, 1024) < 0) {
			err("failed to decode event %s", ev->mcv);
			printf("UNKNOWN");
		} else {
			printf("%s", buf);
		}
	}

	printf("\n");
}

static void
usage(void)
{
	rerr("Usage: ovnidump [-x] DIR\n");
	rerr("\n");
	rerr("Dumps the events of the trace to the standard output.\n");
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

	while ((opt = getopt(argc, argv, "hx")) != -1) {
		switch (opt) {
			case 'x':
				hex_mode = 1;
				break;
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
	progname_set("ovnidump");

	parse_args(argc, argv);

	struct model model;
	model_init(&model);

	/* Register all the models */
	if (models_register(&model) != 0) {
		err("failed to register models");
		return -1;
	}

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

	while ((ret = player_step(player)) == 0) {
		emit(&model, player);
	}

	/* Error happened */
	if (ret < 0) {
		err("player_step failed");
		return 1;
	}

	free(trace);
	free(player);

	return 0;
}
