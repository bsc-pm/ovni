/* Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "cfg_file.h"
#include <stdio.h>
#include <string.h>
#include "common.h"

static const char *color_mode[CFG_COLOR_MODE_MAX] = {
	[CFG_CODE]    = "window_in_code_mode",
	[CFG_GRAD]    = "window_in_gradient_mode",
	[CFG_NGRAD]   = "window_in_null_gradient_mode",
	[CFG_ALTGRAD] = "window_in_alternative_gradient_mode",
	[CFG_PUNCT]   = "window_in_punctual_mode"
};

static const char *semantic_thread[CFG_SEMANTIC_THREAD_MAX] = {
	[CFG_LAST_EV_VAL] = "Last Evt Val",
	[CFG_NEXT_EV_VAL] = "Next Evt Val"
};

static int
write_cfg(FILE *f, struct cfg_file *cf)
{
	fprintf(f, "#ParaverCFG\n");
	fprintf(f, "ConfigFile.Version: 3.4\n");
	fprintf(f, "ConfigFile.NumWindows: 1\n");

	if (cf->desc[0] != '\0') {
		fprintf(f, "ConfigFile.BeginDescription\n");
		fprintf(f, "%s\n", cf->desc);
		fprintf(f, "ConfigFile.EndDescription\n");
	}

	fprintf(f, "\n");
	fprintf(f, "\n");
	fprintf(f, "################################################################################\n");
	fprintf(f, "< NEW DISPLAYING WINDOW %s >\n", cf->title);
	fprintf(f, "################################################################################\n");
	fprintf(f, "window_name %s\n", cf->title);
	fprintf(f, "window_type single\n");
	fprintf(f, "window_id 1\n");
	fprintf(f, "window_position_x 100\n");
	fprintf(f, "window_position_y 100\n");
	fprintf(f, "window_width 600\n");
	fprintf(f, "window_height 150\n");
	fprintf(f, "window_comm_lines_enabled true\n");
	fprintf(f, "window_flags_enabled false\n");
	fprintf(f, "window_noncolor_mode true\n");
	fprintf(f, "window_color_mode %s\n", color_mode[cf->color_mode]);
	fprintf(f, "window_logical_filtered true\n");
	fprintf(f, "window_physical_filtered false\n");
	fprintf(f, "window_comm_fromto true\n");
	fprintf(f, "window_comm_tagsize true\n");
	fprintf(f, "window_comm_typeval true\n");
	fprintf(f, "window_units Microseconds\n");
	fprintf(f, "window_maximum_y 1000.0\n");
	fprintf(f, "window_minimum_y 1.0\n");
	fprintf(f, "window_compute_y_max true\n");
	fprintf(f, "window_level thread\n");
	fprintf(f, "window_scale_relative 1.000000000000\n");
	fprintf(f, "window_end_time_relative 1.000000000000\n");
	fprintf(f, "window_object appl { 1, { All } }\n");
	fprintf(f, "window_begin_time_relative 0.000000000000\n");
	fprintf(f, "window_open true\n");
	fprintf(f, "window_drawmode draw_randnotzero\n");
	fprintf(f, "window_drawmode_rows draw_randnotzero\n");
	fprintf(f, "window_pixel_size 1\n");
	fprintf(f, "window_labels_to_draw 1\n");

	/* We only need to change the thread functions for now. Notice it is one
	 * big line split into multiple fprintf calls for readability. */
	fprintf(f, "window_selected_functions { 14, { ");
		fprintf(f, "{cpu, Active Thd}, ");
		fprintf(f, "{appl, Adding}, ");
		fprintf(f, "{task, Adding}, ");
		fprintf(f, "{thread, %s}, ", semantic_thread[cf->semantic_thread]);
		fprintf(f, "{node, Adding}, ");
		fprintf(f, "{system, Adding}, ");
		fprintf(f, "{workload, Adding}, ");
		fprintf(f, "{from_obj, All}, ");
		fprintf(f, "{to_obj, All}, ");
		fprintf(f, "{tag_msg, All}, ");
		fprintf(f, "{size_msg, All}, ");
		fprintf(f, "{bw_msg, All}, ");
		fprintf(f, "{evt_type, =}, ");
		fprintf(f, "{evt_value, All} ");
	fprintf(f, "} }\n");

	/* Same for the compose functions, but for now no need to change them */
	fprintf(f, "window_compose_functions { 9, { ");
		fprintf(f, "{compose_cpu, As Is}, ");
		fprintf(f, "{compose_appl, As Is}, ");
		fprintf(f, "{compose_task, As Is}, ");
		fprintf(f, "{compose_thread, As Is}, ");
		fprintf(f, "{compose_node, As Is}, ");
		fprintf(f, "{compose_system, As Is}, ");
		fprintf(f, "{compose_workload, As Is}, ");
		fprintf(f, "{topcompose1, As Is}, ");
		fprintf(f, "{topcompose2, As Is} ");
	fprintf(f, "} }\n");

	fprintf(f, "window_filter_module evt_type 1 %ld\n", cf->type);
	fprintf(f, "window_filter_module evt_type_label 1 \"%s\"\n", cf->title);

	return 0;
}

void
cfg_file_init(struct cfg_file *cf, long type, const char *title)
{
	memset(cf, 0, sizeof(struct cfg_file));

	cf->type = type;

	if (snprintf(cf->title, MAX_LABEL, "%s", title) >= MAX_LABEL)
		warn("cfg title truncated: %s", title);
}

void
cfg_file_desc(struct cfg_file *cf, const char *desc)
{
	if (snprintf(cf->desc, MAX_LABEL, "%s", desc) >= MAX_LABEL)
		warn("cfg description truncated: %s", desc);
}

void
cfg_file_color_mode(struct cfg_file *cf, enum cfg_color_mode cm)
{
	cf->color_mode = cm;
}

void
cfg_file_semantic_thread(struct cfg_file *cf, enum cfg_semantic_thread st)
{
	cf->semantic_thread = st;
}

/**
 * Writes the cfg_file @param cf to the file at @param path.
 *
 * It creates the directories leading to @param path if needed. If a file
 * already exists, it gets overwritten.
 */
int
cfg_file_write(struct cfg_file *cf, const char *path)
{
	if (mkpath(path, 0755, 0) != 0) {
		err("cannot create path to %s:", path);
		return -1;
	}

	FILE *f = fopen(path, "w");
	if (f == NULL) {
		err("fopen %s failed:", path);
		return -1;
	}

	if (write_cfg(f, cf) != 0) {
		err("writting configuration file %s failed", path);
		return -1;
	}

	fclose(f);

	return 0;
}
