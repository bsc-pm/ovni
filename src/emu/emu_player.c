#include "emu_player.h"

#include "heap.h"
#include "utlist.h"

/*
 * heap_node_compare_t - comparison function, returns:

 * 	> 0 if a > b
 * 	< 0 if a < b
 * 	= 0 if a == b
 *
 * Invert the comparison function to get a min-heap instead
 */
static inline int
stream_cmp(heap_node_t *a, heap_node_t *b)
{
	struct emu_stream *sa, *sb;

	sa = heap_elem(a, struct emu_stream, hh);
	sb = heap_elem(b, struct emu_stream, hh);

	int64_t ca = emu_stream_lastclock(sa);
	int64_t cb = emu_stream_lastclock(sb);

	/* Return the opposite, so we have min-heap */
	if (ca < cb)
		return +1;
	else if (ca > cb)
		return -1;
	else
		return 0;
}

static int
step_stream(struct emu_player *player, struct emu_stream *stream)
{
	if (!stream->active)
		return +1;

	int ret = emu_stream_step(stream);

	if (ret < 0) {
		err("step_stream: cannot step stream '%s'\n",
				stream->relpath);
		return -1;
	} else if (ret > 0) {
		return ret;
	}

	heap_insert(&player->heap, &stream->hh, &stream_cmp);

	return 0;
}

int
emu_player_init(struct emu_player *player, struct emu_trace *trace)
{
	memset(player, 0, sizeof(struct emu_player));

	heap_init(&player->heap);

	player->first_event = 1;
	player->stream = NULL;

	/* Load initial streams and events */
	struct emu_stream *stream;
	DL_FOREACH(trace->streams, stream) {
		int ret = step_stream(player, stream);
		if (ret > 0) {
			/* No more events */
			continue;
		} else if (ret < 0) {
			err("emu_player_init: step_stream failed\n");
			return -1;
		}
	}

	return 0;
}

static int
update_clocks(struct emu_player *player, struct emu_stream *stream)
{
	/* This can happen if two events are not ordered in the stream, but the
	 * emulator picks other events in the middle. Example:
	 *
	 * Stream A: 10 3 ...
	 * Stream B: 5 12
	 *
	 * emulator output:
	 * 5
	 * 10
	 * 3  -> error!
	 * 12
	 * ...
	 * */
	int64_t sclock = emu_stream_lastclock(stream);

	if (player->first_event) {
		player->first_event = 0;
		player->firstclock = sclock;
		player->lastclock = sclock;
	}

	if (sclock < player->lastclock) {
		err("backwards jump in time %ld -> %ld in stream '%s'\n",
				player->lastclock, sclock, stream->relpath);
		return -1;
	}

	player->lastclock = sclock;
	player->deltaclock = player->lastclock - player->firstclock;

	return 0;
}

/* Returns -1 on error, +1 if there are no more events and 0 if next event
 * loaded properly */
int
emu_player_step(struct emu_player *player)
{
	/* Add the stream back if still active */
	if (player->stream != NULL && step_stream(player, player->stream) < 0) {
		err("player_step: step_stream() failed\n");
		return -1;
	}

	/* Extract the next stream based on the lastclock */
	heap_node_t *node = heap_pop_max(&player->heap, stream_cmp);

	/* No more streams */
	if (node == NULL)
		return +1;

	struct emu_stream *stream = heap_elem(node, struct emu_stream, hh);

	if (stream == NULL) {
		err("emu_player_step: heap_elem() returned NULL\n");
		return -1;
	}

	if (update_clocks(player, stream) != 0) {
		err("emu_player_step: update_clocks() failed\n");
		return -1;
	}

	player->stream = stream;

	struct ovni_ev *oev = emu_stream_ev(stream);
	int64_t sclock = emu_stream_evclock(stream, oev);
	emu_ev(&player->ev, oev, sclock, player->deltaclock);

	return 0;
}

struct emu_ev *
emu_player_ev(struct emu_player *player)
{
	return &player->ev;
}

struct emu_stream *
emu_player_stream(struct emu_player *player)
{
	return player->stream;
}
