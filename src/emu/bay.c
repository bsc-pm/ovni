#define ENABLE_DEBUG

#include "bay.h"

#include "common.h"
#include "uthash.h"
#include "utlist.h"

/* Called from the channel when it becomes dirty */
static int
cb_chan_is_dirty(struct chan *chan, void *arg)
{
	UNUSED(chan);
	struct bay_chan *bchan = arg;
	struct bay *bay = bchan->bay;

	if (bchan->is_dirty) {
		err("channel %s already on dirty list\n", chan->name);
		return -1;
	}

	DL_APPEND(bay->dirty, bchan);
	return 0;
}

static struct bay_chan *
find_bay_chan(struct bay *bay, const char *name)
{
	struct bay_chan *bchan = NULL;
	HASH_FIND_STR(bay->channels, name, bchan);

	return bchan;
}

struct chan *
bay_find(struct bay *bay, const char *name)
{
	struct bay_chan *bchan = find_bay_chan(bay, name);

	if (bchan != NULL)
		return bchan->chan;
	else
		return NULL;
}

int
bay_register(struct bay *bay, struct chan *chan)
{
	struct bay_chan *bchan = find_bay_chan(bay, chan->name);
	if (bchan != NULL) {
		err("bay_register: channel %s already registered\n",
				chan->name);
		return -1;
	}

	bchan = calloc(1, sizeof(struct bay_chan));
	if (bchan == NULL) {
		err("bay_register: calloc failed\n");
		return -1;
	}

	bchan->chan = chan;
	bchan->bay = bay;
	chan_set_dirty_cb(chan, cb_chan_is_dirty, bchan);

	/* Add to hash table */
	HASH_ADD_STR(bay->channels, chan->name, bchan);

	return 0;
}

int
bay_remove(struct bay *bay, struct chan *chan)
{
	struct bay_chan *bchan = find_bay_chan(bay, chan->name);
	if (bchan == NULL) {
		err("bay_remove: channel %s not registered\n",
				chan->name);
		return -1;
	}

	if (bchan->is_dirty) {
		err("bay_remove: cannot remote bay channel %s in dirty state\n",
				chan->name);
		return -1;
	}

	chan_set_dirty_cb(chan, NULL, NULL);

	HASH_DEL(bay->channels, bchan);
	free(bchan);

	return 0;
}

int
bay_add_cb(struct bay *bay, struct chan *chan, bay_cb_func_t func, void *arg)
{
	if (func == NULL) {
		err("bay_add_cb: func is NULL\n");
		return -1;
	}

	struct bay_chan *bchan = find_bay_chan(bay, chan->name);
	if (bchan == NULL) {
		err("bay_add_cb: cannot find channel %s in bay\n", chan->name);
		return -1;
	}

	struct bay_cb *cb = calloc(1, sizeof(struct bay_cb));

	if (cb == NULL) {
		err("bay_add_cb: calloc failed\n");
		return -1;
	}

	cb->func = func;
	cb->arg = arg;

	DL_APPEND(bchan->cb, cb);
	bchan->ncallbacks++;
	return 0;
}

void
bay_init(struct bay *bay)
{
	memset(bay, 0, sizeof(struct bay));
	bay->state = BAY_READY;
}

static int
propagate_chan(struct bay_chan *bchan)
{
	dbg("- propagating dirty channel %s\n", bchan->chan->name);

	struct bay_cb *cur = NULL;
	struct bay_cb *tmp = NULL;
	DL_FOREACH_SAFE(bchan->cb, cur, tmp) {
		if (cur->func(bchan->chan, cur->arg) != 0) {
			err("propagate_chan: callback failed\n");
			return -1;
		}
	}

	return 0;
}

int
bay_propagate(struct bay *bay)
{
	dbg("-- propagating channels begins\n");
	struct bay_chan *cur = NULL;
	struct bay_chan *tmp = NULL;
	DL_FOREACH_SAFE(bay->dirty, cur, tmp) {
		/* May add more dirty channels */
		if (propagate_chan(cur) != 0) {
			err("bay_propagate: propagate_chan failed\n");
			return -1;
		}
	}

	/* Flush channels after running all the dirty callbacks, so we
	 * capture any potential double write when running the
	 * callbacks */
	DL_FOREACH_SAFE(bay->dirty, cur, tmp) {
		if (chan_flush(cur->chan) != 0) {
			err("bay_propagate: chan_flush failed\n");
			return -1;
		}
		cur->is_dirty = 0;
	}

	bay->dirty = NULL;

	dbg("-- propagating channels ends\n");

	return 0;
}

//int
//bay_emit(struct bay *bay)
//{
//	for (chan in dirty channels) {
//		/* May add more dirty channels */
//		emit_chan(chan);
//	}
//
//	return 0;
//}
