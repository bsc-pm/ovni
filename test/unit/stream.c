/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
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
write_dummy_json(const char *path)
{
	const char *json = "{ \"version\" : 3 }";
	FILE *f = fopen(path, "w");

	if (f == NULL)
		die("fopen json failed:");

	if (fwrite(json, strlen(json), 1, f) != 1)
		die("fwrite json failed:");

	fclose(f);
}

static void
test_ok(void)
{
	OK(mkdir("ok", 0755));

	const char *fname = "ok/stream.obs";
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

	write_dummy_json("ok/stream.json");

	struct stream stream;
	OK(stream_load(&stream, ".", "ok"));

	if (stream.active)
		die("stream is active");

	err("OK");
}

static void
test_bad(void)
{
	OK(mkdir("bad", 0755));

	const char *fname = "bad/stream.obs";
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

	write_dummy_json("bad/stream.json");

	struct stream stream;
	ERR(stream_load(&stream, ".", "bad"));

	err("OK");
}

int main(void)
{
	test_ok();
	test_bad();

	return 0;
}
