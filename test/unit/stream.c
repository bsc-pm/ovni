/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include "emu/stream.h"
#include "ovni.h"
#include "unittest.h"

static void
test_ok(char *fname)
{
	FILE *f = fopen(fname, "w");

	if (f == NULL)
		die("fopen failed:");

	/* Write bogus header */
	struct ovni_stream_header header;
	memcpy(&header.magic, OVNI_STREAM_MAGIC, 4);
	header.version = OVNI_STREAM_VERSION;

	if (fwrite(&header, sizeof(header), 1, f) != 1)
		die("fwrite failed:");

	fclose(f);

	struct stream stream;
	const char *relpath = &fname[5];
	OK(stream_load(&stream, "/tmp", relpath));

	if (stream.active)
		die("stream is active");

	err("OK");
}

static void
test_bad(char *fname)
{
	FILE *f = fopen(fname, "w");

	if (f == NULL)
		die("fopen failed:");

	/* Write bogus header */
	struct ovni_stream_header header;
	memcpy(&header.magic, OVNI_STREAM_MAGIC, 4);
	header.version = 1234; /* Wrong version */

	if (fwrite(&header, sizeof(header), 1, f) != 1)
		die("fwrite failed:");

	fclose(f);

	struct stream stream;
	const char *relpath = &fname[5];
	ERR(stream_load(&stream, "/tmp", relpath));

	err("OK");
}

int main(void)
{
	/* Create temporary trace file */
	char fname[] = "/tmp/ovni.stream.XXXXXX";
	int fd = mkstemp(fname);
	if (fd < 0)
		die("mkstemp failed:");

	test_ok(fname);
	test_bad(fname);

	close(fd);

	return 0;
}
