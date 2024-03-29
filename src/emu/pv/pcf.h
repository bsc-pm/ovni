/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PCF_H
#define PCF_H

#include <stdio.h>
#include "common.h"
#include "uthash.h"

#define MAX_PCF_LABEL 512

enum pcf_values {
	PCF_RESERVED = 1000,
};

struct pcf_value {
	int value;
	char label[MAX_PCF_LABEL];

	UT_hash_handle hh;
};

struct pcf_type {
	int id;
	char label[MAX_PCF_LABEL];

	int nvalues;
	struct pcf_value *values;

	UT_hash_handle hh;
};

struct pcf {
	FILE *f;
	int pcf_ntypes;
	struct pcf_type *types;
};

/* Only used to generate tables */
struct pcf_value_label {
	int value;
	char *label;
};

USE_RET int pcf_open(struct pcf *pcf, char *path);
USE_RET int pcf_close(struct pcf *pcf);

USE_RET struct pcf_type *pcf_find_type(struct pcf *pcf, int type_id);
USE_RET struct pcf_type *pcf_add_type(struct pcf *pcf, int type_id, const char *label);
USE_RET struct pcf_value *pcf_add_value(struct pcf_type *type, int value, const char *label);
USE_RET struct pcf_value *pcf_find_value(struct pcf_type *type, int value);

#endif /* PCF_H */
