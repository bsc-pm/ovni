/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _XOPEN_SOURCE 500

#include "trace.h"
#include <dirent.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "ovni.h"
#include "path.h"
#include "stream.h"
#include "utlist.h"

/* See the nftw(3) manual to see why we need a global variable here:
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/nftw.html */
static struct trace *cur_trace = NULL;

static void
add_stream(struct trace *trace, struct stream *stream)
{
	DL_APPEND(trace->streams, stream);
	trace->nstreams++;
}

static int
load_stream(struct trace *trace, const char *json_path)
{
	struct stream *stream = calloc(1, sizeof(struct stream));

	if (stream == NULL) {
		err("calloc failed:");
		return -1;
	}

	/* The json_path must end in .../stream.json, so remove it */
	char path[PATH_MAX];
	if (path_copy(path, json_path) != 0) {
		err("path_copy failed");
		return -1;
	}
	path_dirname(path);

	int offset = (int) strlen(trace->tracedir);
	const char *relpath = path + offset;

	/* Skip begin slashes */
	while (relpath[0] == '/') relpath++;

	if (stream_load(stream, trace->tracedir, relpath) != 0) {
		err("emu_steam_load failed");
		return -1;
	}

	add_stream(trace, stream);

	return 0;
}

static int
is_stream(const char *fpath)
{
	const char *filename = path_filename(fpath);

	if (strcmp(filename, "stream.json") == 0)
		return 1;

	return 0;
}

static int
cb_nftw(const char *fpath, const struct stat *sb,
		int typeflag, struct FTW *ftwbuf)
{
	UNUSED(sb);
	UNUSED(ftwbuf);

	if (typeflag != FTW_F)
		return 0;

	if (!is_stream(fpath))
		return 0;

	return load_stream(cur_trace, fpath);
}

static int
cmp_streams(struct stream *a, struct stream *b)
{
	return strcmp(a->relpath, b->relpath);
}

int
trace_load(struct trace *trace, const char *tracedir)
{
	memset(trace, 0, sizeof(struct trace));

	cur_trace = trace;

	if (snprintf(trace->tracedir, PATH_MAX, "%s", tracedir) >= PATH_MAX) {
		err("path too long: %s", tracedir);
		return -1;
	}

	/* Remove trailing slashes from tracedir */
	path_remove_trailing(trace->tracedir);
	tracedir = trace->tracedir;

	/* Try to open the directory to catch permission errors */
	DIR *dir = opendir(tracedir);
	if (dir == NULL) {
		err("cannot open \"%s\":", tracedir);
		return -1;
	}

	if (closedir(dir) != 0) {
		err("closedir failed:");
		return -1;
	}

	/* Search recursively all streams in the trace directory */
	if (nftw(tracedir, cb_nftw, 50, 0) != 0) {
		err("nftw failed");
		return -1;
	}

	cur_trace = NULL;

	/* Sort the streams */
	DL_SORT(trace->streams, cmp_streams);

	info("loaded %ld streams", trace->nstreams);

	return 0;
}
