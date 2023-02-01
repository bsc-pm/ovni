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

	if (stream->size < (int64_t) sizeof(struct ovni_stream_header)) {
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

	dbg("emu_stream_load: loading %s\n", stream->relpath);

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
	stream->usize = stream->size - stream->offset;

	if (stream->offset < stream->size) {
		stream->active = 1;
	} else if (stream->offset == stream->size) {
		err("warning: stream '%s' has zero events\n", stream->relpath);
		stream->active = 0;
	} else {
		err("emu_stream_load: impossible, offset %ld bigger than size %ld\n",
				stream->offset, stream->size);
		return -1;
	}

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

int
emu_stream_clkoff_set(struct emu_stream *stream, int64_t clkoff)
{
	if (stream->cur_ev) {
		die("emu_stream_clkoff_set: cannot set clokoff in started stream '%s'\n",
				stream->relpath);
		return -1;
	}

	if (stream->clock_offset != 0) {
		err("emu_stream_clkoff_set: stream '%s' already has a clock offset\n",
				stream->relpath);
		return -1;
	}

	stream->clock_offset = clkoff;

	return 0;
}

struct ovni_ev *
emu_stream_ev(struct emu_stream *stream)
{
	return stream->cur_ev;
}

int64_t
emu_stream_evclock(struct emu_stream *stream, struct ovni_ev *ev)
{
	return (int64_t) ovni_ev_get_clock(ev) + stream->clock_offset;
}

int64_t
emu_stream_lastclock(struct emu_stream *stream)
{
	return stream->lastclock;
}

int
emu_stream_step(struct emu_stream *stream)
{
	if (!stream->active) {
		err("emu_stream_step: stream is inactive, cannot step\n");
		return -1;
	}

	/* Only step the offset if we have loaded an event */
	if (stream->cur_ev != NULL) {
		stream->offset += ovni_ev_size(stream->cur_ev);

		/* It cannot pass the size, otherwise we are reading garbage */
		if (stream->offset > stream->size) {
			err("emu_stream_step: stream offset %ld exceeds size %ld\n",
					stream->offset, stream->size);
			return -1;
		}

		/* We have reached the end */
		if (stream->offset == stream->size) {
			stream->active = 0;
			stream->cur_ev = NULL;
			return +1;
		}
	}

	stream->cur_ev = (struct ovni_ev *) &stream->buf[stream->offset];

	/* Ensure the event fits */
	if (stream->offset + ovni_ev_size(stream->cur_ev) > stream->size) {
		err("emu_stream_step: stream '%s' ends with incomplete event\n",
				stream->relpath);
		return -1;
	}

	/* Ensure the clock grows monotonically */
	int64_t clock = emu_stream_evclock(stream, stream->cur_ev);
	if (clock < stream->lastclock) {
		err("clock goes backwards %ld -> %ld in stream '%s' at offset %ld\n",
				stream->lastclock,
				clock,
				stream->relpath,
				stream->offset);
		return -1;
	}
	stream->lastclock = clock;

	return 0;
}

double
emu_stream_progress(struct emu_stream *stream)
{
	if (stream->usize == 0)
		return 1.0;

	int64_t uoffset = stream->offset - sizeof(struct ovni_stream_header);
	double prog = (double) uoffset / (double) stream->usize;
	return prog;
}
