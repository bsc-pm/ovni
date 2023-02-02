/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PRF_H
#define PRF_H

#include <stdio.h>

#define MAX_PRF_LABEL 512

struct prf_row {
	char label[MAX_PRF_LABEL];
	int set;
};

struct prf {
	FILE *f;
	long nrows;
	struct prf_row *rows;
};

int prf_open(struct prf *prf, const char *path, long nrows);
int prf_add(struct prf *prf, long index, const char *name);
int prf_close(struct prf *prf);


#endif /* PRF_H */
