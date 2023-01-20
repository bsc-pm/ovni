#define _GNU_SOURCE

#include "emu/emu.h"
#include "common.h"

#include <stdio.h>
#include <unistd.h>

static void
test_ok(char *fname)
{
	FILE *f = fopen(fname, "w");

	if (f == NULL)
		die("fopen failed\n");

	/* Write bogus header */
	struct ovni_stream_header header;
	memcpy(&header.magic, OVNI_STREAM_MAGIC, 4);
	header.version = OVNI_STREAM_VERSION;

	if (fwrite(&header, sizeof(header), 1, f) != 1)
		die("fwrite failed\n");

	fclose(f);

	struct emu_stream stream;
	const char *relpath = &fname[5];
	if (emu_stream_load(&stream, "/tmp", relpath) != 0)
		die("emu_stream_load failed");

	if (stream.active)
		die("stream is active\n");
}

static void
test_bad(char *fname)
{
	FILE *f = fopen(fname, "w");

	if (f == NULL)
		die("fopen failed\n");

	/* Write bogus header */
	struct ovni_stream_header header;
	memcpy(&header.magic, OVNI_STREAM_MAGIC, 4);
	header.version = 1234; /* Wrong version */

	if (fwrite(&header, sizeof(header), 1, f) != 1)
		die("fwrite failed\n");

	fclose(f);

	struct emu_stream stream;
	const char *relpath = &fname[5];
	if (emu_stream_load(&stream, "/tmp", relpath) == 0)
		die("emu_stream_load didn't fail");
}

int main(void)
{
	/* Create temporary trace file */
	char fname[] = "/tmp/ovni.stream.XXXXXX";
	int fd = mkstemp(fname);
	if (fd < 0)
		die("mkstemp failed\n");

	test_ok(fname);
	test_bad(fname);

	close(fd);

}
