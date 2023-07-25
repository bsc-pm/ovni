/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "common.h"
#include "emu/stream.h"
#include "ovni.h"
#include "unittest.h"

static void
test_ok(void)
{
	const char *fname = "stream-ok.obs";
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
	OK(stream_load(&stream, ".", fname));

	if (stream.active)
		die("stream is active");

	err("OK");
}

static void
test_bad(void)
{
	const char *fname = "stream-bad.obs";
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
	ERR(stream_load(&stream, ".", fname));

	err("OK");
}

int main(void)
{
	test_ok();
	test_bad();

	return 0;
}
