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

static int
html_encode(char *dst, int ndst, const char *src)
{
	int j = 0;
	int nsrc = (int) strlen(src);

	for (int i = 0; i < nsrc; i++) {
		/* Simple check */
		if (j + 10 >= ndst) {
			err("not enough room");
			return -1;
		}

		char c = src[i];
		switch (c) {
			case '&':  strcpy(&dst[j], "&amp;");  j += 5; break;
			case '"':  strcpy(&dst[j], "&quot;"); j += 6; break;
			case '\'': strcpy(&dst[j], "&apos;"); j += 6; break;
			case '<':  strcpy(&dst[j], "&lt;");   j += 4; break;
			case '>':  strcpy(&dst[j], "&gt;");   j += 4; break;
			default:   dst[j++] = c; break;
		}
	}

	dst[j] = '\0';

	return 0;
}

static int
print_event(struct model_spec *spec, long i)
{
	struct ev_decl *evdecl = &spec->evlist[i];
	struct ev_spec *evspec = &spec->evspec->alloc[i];

	char name[16];
	if (html_encode(name, 16, evspec->mcv) != 0) {
		err("html_encode failed for %s", evspec->mcv);
		return -1;
	}

	char signature[1024];
	if (html_encode(signature, 1024, evdecl->signature) != 0) {
		err("html_encode failed for %s", evdecl->signature);
		return -1;
	}

	char desc[1024];
	if (html_encode(desc, 1024, evdecl->description) != 0) {
		err("html_encode failed for %s", evdecl->description);
		return -1;
	}

	printf("<dt><a id=\"%s\" href=\"#%s\"><pre>%s</pre></a></dt>\n", name, name, signature);
	printf("<dd>%s</dd>\n", desc);

	return 0;
}

static int
print_model(struct model_spec *spec)
{
	printf("\n");
	printf("## Model %s\n", spec->name);
	printf("\n");
	printf("List of events for the model *%s* with identifier **`%c`** at version `%s`:\n",
			spec->name, spec->model, spec->version);

	printf("<dl>\n");
	for (long j = 0; j < spec->evspec->nevents; j++) {
		if (print_event(spec, j) != 0) {
			err("cannot print event %ld", j);
			return -1;
		}
	}

	printf("</dl>\n");

	return 0;
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
		return 1;
	}

	printf("# Emulator events\n");
	printf("\n");
	printf("This is a exhaustive list of the events recognized by the emulator.\n");
	printf("Built on %s.\n", __DATE__);

	for (int i = 0; i < 256; i++) {
		if (!model.registered[i])
			continue;

		if (print_model(model.spec[i]) != 0) {
			err("cannot print model %c events", i);
			return 1;
		}
	}

	return 0;
}
