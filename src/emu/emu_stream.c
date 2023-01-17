/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "emu_stream.h"

#include "ovni.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

static int
check_stream_header(struct emu_stream *stream)
{
	int ret = 0;

	if (stream->size < sizeof(struct ovni_stream_header)) {
		err("stream '%s': incomplete stream header\n",
				stream->path);
		return -1;
	}

	struct ovni_stream_header *h =
			(struct ovni_stream_header *) stream->buf;

	if (memcmp(h->magic, OVNI_STREAM_MAGIC, 4) != 0) {
		char magic[5];
		memcpy(magic, h->magic, 4);
		magic[4] = '\0';
		err("stream '%s': wrong stream magic '%s' (expected '%s')\n",
				stream->path, magic, OVNI_STREAM_MAGIC);
		ret = -1;
	}

	if (h->version != OVNI_STREAM_VERSION) {
		err("stream '%s': stream version mismatch %u (expected %u)\n",
				stream->path, h->version, OVNI_STREAM_VERSION);
		ret = -1;
	}

	return ret;
}

static int
load_stream_fd(struct emu_stream *stream, int fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0) {
		perror("fstat failed");
		return -1;
	}

	/* Error because it doesn't have the header */
	if (st.st_size == 0) {
		err("load_stream_fd: stream %s is empty\n", stream->path);
		return -1;
	}

	int prot = PROT_READ | PROT_WRITE;
	stream->buf = mmap(NULL, st.st_size, prot, MAP_PRIVATE, fd, 0);

	if (stream->buf == MAP_FAILED) {
		perror("mmap failed");
		return -1;
	}

	stream->size = st.st_size;

	return 0;
}

int
emu_stream_load(struct emu_stream *stream, const char *tracedir, const char *relpath)
{
	int fd;

	if (snprintf(stream->path, PATH_MAX, "%s/%s", tracedir, relpath) >= PATH_MAX) {
		err("emu_stream_load: path too long: %s/%s\n", tracedir, relpath);
		return -1;
	}

	if (snprintf(stream->relpath, PATH_MAX, "%s", relpath) >= PATH_MAX) {
		err("emu_stream_load: path too long: %s\n", relpath);
		return -1;
	}

	err("emu_stream_load: loading %s\n", stream->relpath);

	if ((fd = open(stream->path, O_RDWR)) == -1) {
		err("emu_stream_load: open failed: %s\n", stream->path);
		return -1;
	}

	if (load_stream_fd(stream, fd) != 0) {
		err("emu_stream_load: load_stream_fd failed for stream '%s'\n",
				stream->path);
		return -1;
	}

	if (check_stream_header(stream) != 0) {
		err("emu_stream_load: stream '%s' has bad header\n",
				stream->path);
		return -1;
	}

	stream->offset = sizeof(struct ovni_stream_header);

	if (stream->offset == stream->size)
		stream->active = 0;
	else
		stream->active = 1;

	/* No need to keep the fd open */
	if (close(fd)) {
		perror("close failed");
		return -1;
	}

	return 0;
}

void
emu_stream_data_set(struct emu_stream *stream, void *data)
{
	stream->data = data;
}

void *
emu_stream_data_get(struct emu_stream *stream)
{
	return stream->data;
}
