/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define _XOPEN_SOURCE 500

#include "emu_trace.h"
#include "utlist.h"
#include <stdlib.h>
#include <string.h>
#include <ftw.h>

/* See the nftw(3) manual to see why we need a global variable here:
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/nftw.html */
static struct emu_trace *cur_trace = NULL;

static void
add_stream(struct emu_trace *trace, struct emu_stream *stream)
{
	DL_APPEND(trace->streams, stream);
	trace->nstreams++;
}

static int
load_stream(struct emu_trace *trace, const char *path)
{
	struct emu_stream *stream = calloc(1, sizeof(struct emu_stream));

	if (stream == NULL) {
		perror("calloc failed");
		return -1;
	}

	int offset = strlen(trace->tracedir);
	const char *relpath = path + offset;

	/* Skip begin slashes */
	while (relpath[0] == '/') relpath++;

	if (emu_stream_load(stream, trace->tracedir, relpath) != 0) {
		err("load_stream: emu_steam_load failed\n");
		return -1;
	}

	add_stream(trace, stream);

	return 0;
}

static int
has_suffix(const char *str, const char *suffix)
{
	if (!str || !suffix)
		return 0;

	int lenstr = strlen(str);
	int lensuffix = strlen(suffix);

	if (lensuffix > lenstr)
		return 0;

	const char *p = str + lenstr - lensuffix;
	if (strncmp(p, suffix, lensuffix) == 0)
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

	if (!has_suffix(fpath, ".ovnistream"))
		return 0;

	return load_stream(cur_trace, fpath);
}

int
emu_trace_load(struct emu_trace *trace, const char *tracedir)
{
	memset(trace, 0, sizeof(struct emu_trace));

	cur_trace = trace;

	if (snprintf(trace->tracedir, PATH_MAX, "%s", tracedir) >= PATH_MAX) {
		err("emu_trace_load: path too long: %s\n", tracedir);
		return -1;
	}

	/* Search recursively all streams in the trace directory */
	if (nftw(tracedir, cb_nftw, 50, 0) != 0) {
		err("emu_trace_load: nftw failed\n");
		return -1;
	}

	cur_trace = NULL;

	err("emu_trace_load: loaded %ld streams\n", trace->nstreams);

	return 0;
}
