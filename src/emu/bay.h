#ifndef BAY_H
#define BAY_H

#include "chan.h"
#include "uthash.h"

/* Handle connections between channels and callbacks */

struct bay;
struct bay_cb;
struct bay_chan;

enum bay_cb_type {
	BAY_CB_DIRTY = 0,
	BAY_CB_EMIT,
	BAY_CB_MAX,
};

typedef int (*bay_cb_func_t)(struct chan *chan, void *ptr);

struct bay_cb {
	bay_cb_func_t func;
	void *arg;
	struct bay_chan *bchan;
	int enabled;
	int type;

	/* List of callbacks in one channel */
	struct bay_cb *next;
	struct bay_cb *prev;
};

#define MAX_BAY_NAME 1024

struct bay_chan {
	struct chan *chan;
	int ncallbacks[BAY_CB_MAX];
	struct bay_cb *cb[BAY_CB_MAX];
	struct bay *bay;
	int is_dirty;

	/* Global hash table with all channels in bay */
	UT_hash_handle hh;

	/* Used by dirty list */
	struct bay_chan *next;
	struct bay_chan *prev;
};

enum bay_state {
	BAY_UKNOWN = 0,
	BAY_READY,
	BAY_PROPAGATING,
	BAY_EMITTING,
	BAY_FLUSHING
};

struct bay {
	enum bay_state state;
	struct bay_chan *channels;
	struct bay_chan *dirty;
};

        void bay_init(struct bay *bay);
USE_RET int bay_register(struct bay *bay, struct chan *chan);
USE_RET int bay_remove(struct bay *bay, struct chan *chan);
USE_RET int bay_propagate(struct bay *bay);
USE_RET struct chan *bay_find(struct bay *bay, const char *name);
USE_RET struct bay_cb *bay_add_cb(struct bay *bay, enum bay_cb_type type,
		struct chan *chan, bay_cb_func_t func, void *arg, int enabled);
        void bay_enable_cb(struct bay_cb *cb);
        void bay_disable_cb(struct bay_cb *cb);

#endif /* BAY_H */
