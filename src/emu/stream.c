/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "stream.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ovni.h"
#include "path.h"

static int
check_stream_header(struct stream *stream)
{
	int ret = 0;

	if (stream->size < (int64_t) sizeof(struct ovni_stream_header)) {
		err("stream '%s': incomplete stream header",
				stream->path);
		return -1;
	}

	struct ovni_stream_header *h =
			(struct ovni_stream_header *) stream->buf;

	if (memcmp(h->magic, OVNI_STREAM_MAGIC, 4) != 0) {
		char magic[5];
		memcpy(magic, h->magic, 4);
		magic[4] = '\0';
		err("stream '%s': wrong stream magic '%s' (expected '%s')",
				stream->path, magic, OVNI_STREAM_MAGIC);
		ret = -1;
	}

	if (h->version != OVNI_STREAM_VERSION) {
		err("stream '%s': stream version mismatch %u (expected %u)",
				stream->path, h->version, OVNI_STREAM_VERSION);
		ret = -1;
	}

	return ret;
}

static int
load_stream_fd(struct stream *stream, int fd)
{
	struct stat st;
	if (fstat(fd, &st) < 0) {
		err("fstat failed:");
		return -1;
	}

	/* Error because it doesn't have the header */
	if (st.st_size == 0) {
		err("stream %s is empty", stream->path);
		return -1;
	}

	int prot = PROT_READ | PROT_WRITE;
	stream->buf = mmap(NULL, st.st_size, prot, MAP_PRIVATE, fd, 0);

	if (stream->buf == MAP_FAILED) {
		err("mmap failed:");
		return -1;
	}

	stream->size = st.st_size;

	return 0;
}

int
stream_load(struct stream *stream, const char *tracedir, const char *relpath)
{
	memset(stream, 0, sizeof(struct stream));

	if (snprintf(stream->path, PATH_MAX, "%s/%s", tracedir, relpath) >= PATH_MAX) {
		err("path too long: %s/%s", tracedir, relpath);
		return -1;
	}

	/* Allow loading a trace with empty relpath */
	path_remove_trailing(stream->path);

	if (snprintf(stream->relpath, PATH_MAX, "%s", relpath) >= PATH_MAX) {
		err("path too long: %s", relpath);
		return -1;
	}

	dbg("loading %s", stream->relpath);

	int fd;
	if ((fd = open(stream->path, O_RDWR)) == -1) {
		err("open %s failed:", stream->path);
		return -1;
	}

	if (load_stream_fd(stream, fd) != 0) {
		err("load_stream_fd failed for stream '%s'",
				stream->path);
		return -1;
	}

	if (check_stream_header(stream) != 0) {
		err("stream '%s' has bad header",
				stream->path);
		return -1;
	}

	stream->offset = sizeof(struct ovni_stream_header);
	stream->usize = stream->size - stream->offset;

	if (stream->offset < stream->size) {
		stream->active = 1;
	} else if (stream->offset == stream->size) {
		warn("stream '%s' has zero events", stream->relpath);
		stream->active = 0;
	} else {
		err("impossible, offset %lld bigger than size %lld",
				stream->offset, stream->size);
		return -1;
	}

	/* No need to keep the fd open */
	if (close(fd)) {
		err("close failed:");
		return -1;
	}

	return 0;
}

void
stream_data_set(struct stream *stream, void *data)
{
	stream->data = data;
}

void *
stream_data_get(struct stream *stream)
{
	return stream->data;
}

int
stream_clkoff_set(struct stream *stream, int64_t clkoff)
{
	if (stream->cur_ev) {
		err("cannot set clokoff in started stream '%s'",
				stream->relpath);
		return -1;
	}

	if (stream->clock_offset != 0) {
		err("stream '%s' already has a clock offset",
				stream->relpath);
		return -1;
	}

	stream->clock_offset = clkoff;

	return 0;
}

struct ovni_ev *
stream_ev(struct stream *stream)
{
	return stream->cur_ev;
}

int64_t
stream_evclock(struct stream *stream, struct ovni_ev *ev)
{
	return (int64_t) ovni_ev_get_clock(ev) + stream->clock_offset;
}

int64_t
stream_lastclock(struct stream *stream)
{
	return stream->lastclock;
}

int
stream_step(struct stream *stream)
{
	if (!stream->active) {
		err("stream is inactive, cannot step");
		return -1;
	}

	/* Only step the offset if we have loaded an event */
	if (stream->cur_ev != NULL) {
		stream->offset += ovni_ev_size(stream->cur_ev);

		/* It cannot pass the size, otherwise we are reading garbage */
		if (stream->offset > stream->size) {
			err("stream offset %lld exceeds size %lld",
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
		err("stream '%s' ends with incomplete event",
				stream->relpath);
		return -1;
	}

	int64_t clock = stream_evclock(stream, stream->cur_ev);

	/* Ensure the clock grows monotonically if unsorted flag not set */
	if (stream->unsorted == 0) {
		if (clock < stream->lastclock) {
			err("clock goes backwards %lld -> %lld in stream '%s' at offset %lld",
					stream->lastclock,
					clock,
					stream->relpath,
					stream->offset);
			return -1;
		}
	}

	stream->deltaclock = clock - stream->lastclock;
	stream->lastclock = clock;

	return 0;
}

void
stream_progress(struct stream *stream, int64_t *done, int64_t *total)
{
	*done = stream->offset - sizeof(struct ovni_stream_header);
	*total = stream->usize;
}

void
stream_allow_unsorted(struct stream *stream)
{
	stream->unsorted = 1;
}
