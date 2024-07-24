/* Copyright (c) 2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "model_evspec.h"
#include "model.h"
#include "ev_spec.h"
#include <string.h>

int
model_evspec_init(struct model_evspec *evspec, struct model_spec *spec)
{
	memset(evspec, 0, sizeof(struct model_evspec));

	/* Count events */
	for (long i = 0; spec->evlist[i].signature != NULL; i++)
		evspec->nevents++;

	if (evspec->nevents == 0) {
		err("no events defined in model %s", spec->name);
		return -1;
	}

	/* Preallocate a contiguous map, as we know the size */
	evspec->alloc = calloc((size_t) evspec->nevents, sizeof(struct ev_spec));
	if (evspec->alloc == NULL) {
		err("calloc failed:");
		return -1;
	}

	for (long i = 0; spec->evlist[i].signature != NULL; i++) {
		struct ev_decl *evdecl = &spec->evlist[i];
		struct ev_spec *s = &evspec->alloc[i];

		if (ev_spec_compile(s, evdecl) != 0) {
			err("cannot compile event declaration %s",
					evdecl->signature);
			return -1;
		}

		/* Ensure is not duplicated */
		struct ev_spec *dup = model_evspec_find(evspec, s->mcv);

		if (dup != NULL) {
			err("duplicated MCV %s in model %s",
					evdecl->signature, spec->name);
			return -1;
		}

		/* Ensure the model character in the declaration matches
		 * the registered model */
		if (s->mcv[0] != spec->model) {
			err("bad MCV '%s' for model %s, should start with '%c'",
					evdecl->signature,
					spec->name,
					spec->model);
			return -1;
		}


		HASH_ADD_STR(evspec->spec, mcv, s);
	}

	return 0;
}

struct ev_spec *
model_evspec_find(struct model_evspec *evspec, char *mcv)
{
	struct ev_spec *s = NULL;
	HASH_FIND_STR(evspec->spec, mcv, s);
	return s;
}
