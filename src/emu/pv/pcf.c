/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "pcf.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

/* clang-format off */

const char *pcf_def_header =
	"DEFAULT_OPTIONS\n"
	"\n"
	"LEVEL               THREAD\n"
	"UNITS               NANOSEC\n"
	"LOOK_BACK           100\n"
	"SPEED               1\n"
	"FLAG_ICONS          ENABLED\n"
	"NUM_OF_STATE_COLORS 1000\n"
	"YMAX_SCALE          37\n"
	"\n"
	"\n"
	"DEFAULT_SEMANTIC\n"
	"\n"
	"THREAD_FUNC         State As Is\n";

#define RGB(r, g, b) (r<<16 | g<<8 | b)

/* Define colors for the trace */
#define DEEPBLUE  RGB(  0,   0, 255)
#define LIGHTGREY RGB(217, 217, 217)
#define RED       RGB(230,  25,  75)
#define GREEN     RGB(60,  180,  75)
#define YELLOW    RGB(255, 225,  25)
#define ORANGE    RGB(245, 130,  48)
#define PURPLE    RGB(145,  30, 180)
#define CYAN      RGB( 70, 240, 240)
#define MAGENTA   RGB(240, 50,  230)
#define LIME      RGB(210, 245,  60)
#define PINK      RGB(250, 190, 212)
#define TEAL      RGB(  0, 128, 128)
#define LAVENDER  RGB(220, 190, 255)
#define BROWN     RGB(170, 110,  40)
#define BEIGE     RGB(255, 250, 200)
#define MAROON    RGB(128,   0,   0)
#define MINT      RGB(170, 255, 195)
#define OLIVE     RGB(128, 128,   0)
#define APRICOT   RGB(255, 215, 180)
#define NAVY      RGB(  0,   0, 128)
#define BLUE      RGB(  0, 130, 200)
#define GREY      RGB(128, 128, 128)
#define BLACK     RGB(  0,   0,   0)

const uint32_t pcf_def_palette[] = {
	BLACK,		/* (never shown anyways) */
	BLUE,		/* runtime */
	LIGHTGREY,	/* busy wait */
	RED,		/* task */
	GREEN,
	YELLOW, ORANGE, PURPLE, CYAN, MAGENTA, LIME, PINK,
	TEAL, GREY, LAVENDER, BROWN, BEIGE, MAROON, MINT,
	OLIVE, APRICOT, NAVY, DEEPBLUE
};

const uint32_t *pcf_palette = pcf_def_palette;
const int pcf_palette_len = ARRAYLEN(pcf_def_palette);

/* clang-format on */

/* ----------------------------------------------- */

static void
decompose_rgb(uint32_t col, uint8_t *r, uint8_t *g, uint8_t *b)
{
	*r = (col >> 16) & 0xff;
	*g = (col >> 8) & 0xff;
	*b = (col >> 0) & 0xff;
}

static void
write_header(FILE *f)
{
	fprintf(f, "%s", pcf_def_header);
}

static void
write_colors(FILE *f, const uint32_t *palette, int n)
{
	fprintf(f, "\n\n");
	fprintf(f, "STATES_COLOR\n");

	for (int i = 0; i < n; i++) {
		uint8_t r, g, b;
		decompose_rgb(palette[i], &r, &g, &b);
		fprintf(f, "%-3d {%3d, %3d, %3d}\n", i, r, g, b);
	}
}

static void
write_type(FILE *f, struct pcf_type *type)
{
	fprintf(f, "\n\n");
	fprintf(f, "EVENT_TYPE\n");
	fprintf(f, "0 %-10d %s\n", type->id, type->label);
	fprintf(f, "VALUES\n");

	for (struct pcf_value *v = type->values; v != NULL; v = v->hh.next)
		fprintf(f, "%-4d %s\n", v->value, v->label);
}

static void
write_types(struct pcf *pcf)
{
	for (struct pcf_type *t = pcf->types; t != NULL; t = t->hh.next)
		write_type(pcf->f, t);
}

/** Open the given PCF file and create the default events. */
int
pcf_open(struct pcf *pcf, char *path)
{
	memset(pcf, 0, sizeof(*pcf));

	pcf->f = fopen(path, "w");

	if (pcf->f == NULL) {
		err("cannot open PCF file '%s':", path);
		return -1;
	}

	return 0;
}

struct pcf_type *
pcf_find_type(struct pcf *pcf, int type_id)
{
	struct pcf_type *type;

	HASH_FIND_INT(pcf->types, &type_id, type);

	return type;
}

/** Creates a new pcf_type with the given type_id and label. The label
 * can be disposed after return.
 *
 * @return The pcf_type created.
 */
struct pcf_type *
pcf_add_type(struct pcf *pcf, int type_id, const char *label)
{
	struct pcf_type *pcftype = pcf_find_type(pcf, type_id);

	if (pcftype != NULL) {
		err("PCF type %d already defined", type_id);
		return NULL;
	}

	pcftype = calloc(1, sizeof(struct pcf_type));
	if (pcftype == NULL) {
		err("calloc failed:");
		return NULL;
	}

	pcftype->id = type_id;
	pcftype->values = NULL;
	pcftype->nvalues = 0;

	int len = snprintf(pcftype->label, MAX_PCF_LABEL, "%s", label);
	if (len >= MAX_PCF_LABEL) {
		err("PCF type label too long");
		return NULL;
	}

	HASH_ADD_INT(pcf->types, id, pcftype);

	return pcftype;
}

struct pcf_value *
pcf_find_value(struct pcf_type *type, int value)
{
	struct pcf_value *pcfvalue;

	HASH_FIND_INT(type->values, &value, pcfvalue);

	return pcfvalue;
}

/** Adds a new value to the given pcf_type. The label can be disposed
 * after return.
 *
 * @return The new pcf_value created
 */
struct pcf_value *
pcf_add_value(struct pcf_type *type, int value, const char *label)
{
	struct pcf_value *pcfvalue = pcf_find_value(type, value);

	if (pcfvalue != NULL) {
		err("PCF value %d already in type %d", value, type->id);
		return NULL;
	}

	pcfvalue = calloc(1, sizeof(struct pcf_value));
	if (pcfvalue == NULL) {
		err("calloc failed:");
		return NULL;
	}

	pcfvalue->value = value;

	int len = snprintf(pcfvalue->label, MAX_PCF_LABEL, "%s", label);
	if (len >= MAX_PCF_LABEL) {
		err("PCF value label too long");
		return NULL;
	}

	HASH_ADD_INT(type->values, value, pcfvalue);

	type->nvalues++;

	return pcfvalue;
}

/** Writes the defined event and values to the PCF file and closes the file. */
int
pcf_close(struct pcf *pcf)
{
	write_header(pcf->f);
	write_colors(pcf->f, pcf_palette, pcf_palette_len);
	write_types(pcf);

	fclose(pcf->f);
	return 0;
}
