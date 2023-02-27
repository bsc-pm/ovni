/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef KERNEL_PRIV_H
#define KERNEL_PRIV_H

#include "emu.h"
#include "model_cpu.h"
#include "model_thread.h"

/* Private enums */

enum kernel_chan {
	CH_CS = 0,
	CH_MAX,
};

enum kernel_cs_state {
	ST_CSOUT = 3,
};

struct kernel_thread {
	struct model_thread m;
};

struct kernel_cpu {
	struct model_cpu m;
};

int model_kernel_probe(struct emu *emu);
int model_kernel_create(struct emu *emu);
int model_kernel_connect(struct emu *emu);
int model_kernel_event(struct emu *emu);
int model_kernel_finish(struct emu *emu);

#endif /* KERNEL_PRIV_H */
