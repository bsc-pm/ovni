/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "prf.h"

#include <string.h>
#include <stdlib.h>
#include "common.h"

int
prf_open(struct prf *prf, const char *path, long nrows)
{
	memset(prf, 0, sizeof(*prf));

	prf->f = fopen(path, "w");

	if (prf->f == NULL) {
		err("cannot open ROW file '%s':", path);
		return -1;
	}

	prf->nrows = nrows;
	prf->rows = calloc((size_t) nrows, sizeof(struct prf_row));

	if (prf->rows == NULL) {
		err("calloc failed:");
		return -1;
	}

	return 0;
}

int
prf_add(struct prf *prf, long index, const char *label)
{
	if (index < 0 || index >= prf->nrows) {
		err("index out of bounds");
		return -1;
	}

	struct prf_row *row = &prf->rows[index];
	if (row->set) {
		err("row %ld already set to '%s'", index, row->label);
		return -1;
	}

	int n = snprintf(row->label, MAX_PRF_LABEL, "%s", label);

	if (n >= MAX_PRF_LABEL) {
		err("label '%s' too long", label);
		return -1;
	}

	row->set = 1;
	return 0;
}

int
prf_close(struct prf *prf)
{
	for (long i = 0; i < prf->nrows; i++) {
		struct prf_row *row = &prf->rows[i];
		if (!row->set) {
			err("row %ld not set", i);
			return -1;
		}
	}

	FILE *f = prf->f;
	fprintf(f, "LEVEL NODE SIZE 1\n");
	fprintf(f, "hostname\n");
	fprintf(f, "\n");

	fprintf(f, "LEVEL THREAD SIZE %ld\n", prf->nrows);

	for (long i = 0; i < prf->nrows; i++) {
		struct prf_row *row = &prf->rows[i];
		fprintf(f, "%s\n", row->label);
	}

	fclose(f);

	return 0;
}
