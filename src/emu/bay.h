#ifndef BAY_H
#define BAY_H

#include "chan.h"
#include "uthash.h"

/* Handle connections between channels and callbacks */

struct bay;
struct bay_cb;
struct bay_chan;

typedef int (*bay_cb_func_t)(struct chan *chan, void *ptr);

struct bay_cb {
	bay_cb_func_t func;
	void *arg;

	/* List of callbacks in one channel */
	struct bay_cb *next;
	struct bay_cb *prev;
};

#define MAX_BAY_NAME 1024

struct bay_chan {
	struct chan *chan;
	int ncallbacks;
	struct bay_cb *cb;
	struct bay *bay;
	int is_dirty;

	/* Global hash table with all channels in bay */
	UT_hash_handle hh;

	/* Used by dirty list */
	struct bay_chan *next;
	struct bay_chan *prev;
};

enum bay_state {
	BAY_CONFIG = 1,
	BAY_READY,
	BAY_PROPAGATING,
	BAY_PROPAGATED,
	BAY_EMITTING,
	BAY_EMITTED
};

struct bay {
	enum bay_state state;
	struct bay_chan *channels;
	struct bay_chan *dirty;
};

int bay_register(struct bay *bay, struct chan *chan);
int bay_remove(struct bay *bay, struct chan *chan);
struct chan *bay_find(struct bay *bay, const char *name);
int bay_add_cb(struct bay *bay, struct chan *chan, bay_cb_func_t func, void *arg);
void bay_init(struct bay *bay);
int bay_propagate(struct bay *bay);

#endif /* BAY_H */
