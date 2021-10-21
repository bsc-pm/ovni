#ifndef OVNI_CHAN_H
#define OVNI_CHAN_H

#include "emu.h"

void
chan_init(struct ovni_chan *chan, int row, int type, FILE *prv, int64_t *clock);

void
chan_enable(struct ovni_chan *chan, int enabled);

int
chan_is_enabled(struct ovni_chan *chan);

void
chan_set(struct ovni_chan *chan, int st);

void
chan_push(struct ovni_chan *chan, int st);

int
chan_pop(struct ovni_chan *chan, int expected_st);

void
chan_ev(struct ovni_chan *chan, int ev);

int
chan_get_st(struct ovni_chan *chan);

void
chan_emit(struct ovni_chan *chan);

#endif /* OVNI_CHAN_H */
