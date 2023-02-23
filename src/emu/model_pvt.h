/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef MODEL_PRV_H
#define MODEL_PRV_H

#include "emu.h"
#include "pv/pcf.h"

struct model_pvt_spec {
	const int *type;
	const char **prefix;
	const char **suffix;
	const long *flags;
	const struct pcf_value_label (**label)[];
};

#include "model_cpu.h"
#include "model_thread.h"

int model_pvt_connect_cpu(struct emu *emu, const struct model_cpu_spec *spec);
int model_pvt_connect_thread(struct emu *emu, const struct model_thread_spec *spec);

#endif /* MODEL_PRV_H */
