#ifndef OVNI_PRV_H
#define OVNI_PRV_H

#include <stdint.h>
#include "ovni.h"
#include "emu.h"

void
prv_ev(FILE *f, int row, int64_t time, int type, int val);

void
prv_ev_thread_raw(struct ovni_emu *emu, int row, int64_t time, int type, int val);

void
prv_ev_thread(struct ovni_emu *emu, int row, int type, int val);

void
prv_ev_cpu(struct ovni_emu *emu, int row, int type, int val);

void
prv_ev_autocpu(struct ovni_emu *emu, int type, int val);

void
prv_ev_autocpu_raw(struct ovni_emu *emu, int64_t time, int type, int val);

void
prv_header(FILE *f, int nrows);

#endif /* OVNI_PRV_H */
