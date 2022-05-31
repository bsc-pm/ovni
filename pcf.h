/*
 * Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef OVNI_PCF_H
#define OVNI_PCF_H

#include <stdio.h>
#include "uthash.h"

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

void pcf_open(struct pcf_file *pcf, char *path, int chantype);

void pcf_write(struct pcf_file *pcf);

void pcf_close(struct pcf_file *pcf);

struct pcf_type *pcf_find_type(struct pcf_file *pcf, int type_id);

struct pcf_type *pcf_add_type(struct pcf_file *pcf, int type_id,
		const char *label);

struct pcf_value *pcf_add_value(struct pcf_type *type, int value,
		const char *label);

#endif /* OVNI_PCF_H */
