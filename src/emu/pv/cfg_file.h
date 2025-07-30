/* Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef CFG_FILE_H
#define CFG_FILE_H

#include "common.h"

#define MAX_LABEL 1024

/** Controls the "Paint As" option */
enum cfg_color_mode {
	CFG_CODE = 0, /* Default */
	CFG_GRAD,
	CFG_NGRAD,
	CFG_ALTGRAD,
	CFG_PUNCT,
	CFG_COLOR_MODE_MAX
};

/** Controls how segments are painted from events. */
enum cfg_semantic_thread {
	CFG_LAST_EV_VAL = 0, /* Default */
	CFG_NEXT_EV_VAL,
	CFG_SEMANTIC_THREAD_MAX
};

/** Represents a .cfg file on disk. Needed fields to generate the file. */
struct cfg_file {
	char title[MAX_LABEL];
	char desc[MAX_LABEL];
	long type;
	enum cfg_color_mode color_mode;
	enum cfg_semantic_thread semantic_thread;
};

void cfg_file_init(struct cfg_file *cf, long type, const char *title);
void cfg_file_desc(struct cfg_file *cf, const char *desc);
void cfg_file_color_mode(struct cfg_file *cf, enum cfg_color_mode cm);
void cfg_file_semantic_thread(struct cfg_file *cf, enum cfg_semantic_thread st);
USE_RET int cfg_file_write(struct cfg_file *cf, const char *path);

#endif /* CFG_FILE_H */
