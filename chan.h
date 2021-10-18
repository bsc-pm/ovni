#ifndef OVNI_CHAN_H
#define OVNI_CHAN_H

#include "emu.h"

void
chan_th_init(struct ovni_ethread *th,
		struct ovni_chan **update_list,
		enum chan id,
		int track,
		int init_st,
		int enabled,
		int dirty,
		int row,
		FILE *prv,
		int64_t *clock);

void
chan_cpu_init(struct ovni_cpu *cpu, struct ovni_chan **update_list, enum chan id, int track, int row, FILE *prv, int64_t *clock);

void
chan_enable(struct ovni_chan *chan, int enabled);

void
chan_disable(struct ovni_chan *chan);

int
chan_is_enabled(struct ovni_chan *chan);

void
chan_set(struct ovni_chan *chan, int st);

void
chan_enable_and_set(struct ovni_chan *chan, int st);

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
