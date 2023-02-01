//#define ENABLE_DEBUG

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

	if (bay->state != BAY_READY && bay->state != BAY_PROPAGATING) {
		err("cannot add dirty channel %s in current bay state\n",
				chan->name);
		return -1;
	}

	if (bchan->is_dirty) {
		err("channel %s already on dirty list\n", chan->name);
		return -1;
	}

	dbg("adding dirty chan %s", chan->name)
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

	dbg("registered %s", chan->name);

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
bay_add_cb(struct bay *bay, enum bay_cb_type type,
		struct chan *chan, bay_cb_func_t func, void *arg)
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

	DL_APPEND(bchan->cb[type], cb);
	bchan->ncallbacks[type]++;
	return 0;
}

void
bay_init(struct bay *bay)
{
	memset(bay, 0, sizeof(struct bay));
	bay->state = BAY_READY;
}

static int
propagate_chan(struct bay_chan *bchan, enum bay_cb_type type)
{
	char *propname[BAY_CB_MAX] = {
		[BAY_CB_DIRTY] = "dirty",
		[BAY_CB_EMIT] = "emit"
	};

	UNUSED(propname);

	dbg("- propagating channel '%s' phase %s\n",
			bchan->chan->name, propname[type]);

	struct bay_cb *cur = NULL;
	DL_FOREACH(bchan->cb[type], cur) {
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
	struct bay_chan *cur;
	bay->state = BAY_PROPAGATING;
	DL_FOREACH(bay->dirty, cur) {
		/* May add more dirty channels */
		if (propagate_chan(cur, BAY_CB_DIRTY) != 0) {
			err("bay_propagate: propagate_chan failed\n");
			return -1;
		}
	}

	dbg("<> dirty phase complete");

	/* Once the dirty callbacks have been propagated,
	 * begin the emit stage */
	bay->state = BAY_EMITTING;
	DL_FOREACH(bay->dirty, cur) {
		/* May add more dirty channels */
		if (propagate_chan(cur, BAY_CB_EMIT) != 0) {
			err("bay_propagate: propagate_chan failed\n");
			return -1;
		}
	}

	dbg("<> emit phase complete");

	/* Flush channels after running all the dirty and emit
	 * callbacks, so we capture any potential double write when
	 * running the callbacks */
	bay->state = BAY_FLUSHING;
	DL_FOREACH(bay->dirty, cur) {
		if (chan_flush(cur->chan) != 0) {
			err("bay_propagate: chan_flush failed\n");
			return -1;
		}
		cur->is_dirty = 0;
	}

	bay->dirty = NULL;
	bay->state = BAY_READY;

	return 0;
}
