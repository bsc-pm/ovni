/* Copyright (c) 2023-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "common.h"
#include "emu_ev.h"
#include "model.h"
#include "models.h"
#include "model_evspec.h"
#include "ev_spec.h"
#include "ovni.h"
#include "player.h"
#include "stream.h"
#include "trace.h"

static void
print_event(struct model_spec *spec, long i)
{
	struct ev_decl *evdecl = &spec->evlist[i];
	struct ev_spec *evspec = &spec->evspec->alloc[i];

	const char *name = evspec->mcv;

	printf("<dt><a id=\"%s\" href=\"#%s\"><pre>%s</pre></a></dt>\n", name, name, evdecl->signature);
	printf("<dd>%s</dd>\n", evdecl->description);
}

static void
print_model(struct model_spec *spec)
{
	printf("\n");
	printf("## Model %s\n", spec->name);
	printf("\n");
	printf("List of events for the model *%s* with identifier **`%c`** at version `%s`:\n",
			spec->name, spec->model, spec->version);

	printf("<dl>\n");
	for (long j = 0; j < spec->evspec->nevents; j++)
		print_event(spec, j);
	printf("</dl>\n");
}

int
main(void)
{
	progname_set("ovnievents");

	struct model model;
	model_init(&model);

	/* Register all the models */
	if (models_register(&model) != 0) {
		err("failed to register models");
		return -1;
	}

	printf("# Emulator events\n");
	printf("\n");
	printf("This is a exhaustive list of the events recognized by the emulator.\n");
	printf("Built on %s.\n", __DATE__);

	for (int i = 0; i < 256; i++) {
		if (!model.registered[i])
			continue;

		print_model(model.spec[i]);
	}

	return 0;
}
