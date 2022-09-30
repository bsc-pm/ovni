/* Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef OVNI_PCF_H
#define OVNI_PCF_H

#include "uthash.h"
#include <stdio.h>

#define MAX_PCF_LABEL 512

struct pcf_value;
struct pcf_type;

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

struct pcf_file {
	FILE *f;
	int chantype;
	int pcf_ntypes;
	struct pcf_type *types;
};

/* Only used to generate tables */
struct pcf_value_label {
	int value;
	char *label;
};

extern struct pcf_value_label nanos6_ss_values[];

void pcf_open(struct pcf_file *pcf, char *path, int chantype);

void pcf_write(struct pcf_file *pcf);

void pcf_close(struct pcf_file *pcf);

struct pcf_type *pcf_find_type(struct pcf_file *pcf, int type_id);

struct pcf_type *pcf_add_type(struct pcf_file *pcf, int type_id,
		const char *label);

struct pcf_value *pcf_add_value(struct pcf_type *type, int value,
		const char *label);

struct pcf_value *pcf_find_value(struct pcf_type *type, int value);

#endif /* OVNI_PCF_H */
