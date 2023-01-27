/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ust_model.h"

#include "emu.h"

struct model_spec ust_model_spec = {
	.name = "ust",
	.model = 'O',
	.create  = ust_model_create,
	.connect = ust_model_connect,
	.event   = ust_model_event,
	.probe   = ust_model_probe,
};

int
ust_model_create(void *p)
{
	struct emu *emu = emu_get(p);
	UNUSED(emu);

	/* Get paraver traces */
	//oemu->pvt_thread = pvman_new(emu->pvman, "thread");
	//oemu->pvt_cpu = pvman_new(emu->pvman, "cpu");

	return 0;
}
