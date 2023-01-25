/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ovni_model.h"

#include "emu.h"
#include "bay.h"

struct model_spec ovni_model_spec = {
	.name = "ovni",
	.model = 'O',
	.create = ovni_model_create,
	.connect = ovni_model_connect,
	.event = ovni_model_event,
	.probe = ovni_model_probe,
};

static char *th_channels[] = {
	"state",
	"is_running",
	"is_active",
	"tid_active",
	"pid_active",
	"cpu",
	"is_flushing"
};

static char *cpu_channels[] = {
	"tid_running",
	"pid_running",
	"nthreads_running",
	"is_flushing"
};

struct pcf_value_label ovni_state_values[] = {
	{ TH_ST_UNKNOWN, "Unknown" },
	{ TH_ST_RUNNING, "Running" },
	{ TH_ST_PAUSED,  "Paused"  },
	{ TH_ST_DEAD,    "Dead"    },
	{ TH_ST_COOLING, "Cooling" },
	{ TH_ST_WARMING, "Warming" },
	{ -1, NULL },
};

struct pcf_value_label ovni_flush_values[] = {
	{ 0, "None" },
	{ ST_OVNI_FLUSHING, "Flushing" },
	{ ST_TOO_MANY_TH, "Unknown flushing state: Multiple threads running" },
	{ -1, NULL },
};

//	[CHAN_OVNI_PID]         = "PID",
//	[CHAN_OVNI_TID]         = "TID",
//	[CHAN_OVNI_NRTHREADS]   = "Number of RUNNING threads",
//	[CHAN_OVNI_STATE]       = "Execution state",
//	[CHAN_OVNI_APPID]       = "AppID",
//	[CHAN_OVNI_CPU]         = "CPU affinity",
//	[CHAN_OVNI_FLUSH]       = "Flushing state",

static void
create_chan(struct bay *bay, char *group, int64_t gid, char *item)
{
	char name[MAX_CHAN_NAME];
	sprintf(name, "%s.%s%ld.%s", ovni_model_spec.name, group, gid, item);

	struct chan *c = calloc(1, sizeof(struct chan));
	if (c == NULL)
		die("calloc failed\n");

	chan_init(c, CHAN_SINGLE, name);
	if (bay_register(bay, c) != 0)
		die("bay_register failed\n");
}

static void
create_thread(struct bay *bay, int64_t gid)
{
	for (size_t i = 0; i < ARRAYLEN(th_channels); i++)
		create_chan(bay, "thread", gid, th_channels[i]);
}

static void
create_cpu(struct bay *bay, int64_t gid)
{
	for (size_t i = 0; i < ARRAYLEN(cpu_channels); i++)
		create_chan(bay, "cpu", gid, cpu_channels[i]);
}

static void
create_channels(struct emu_system *sys, struct bay *bay)
{
	for (size_t i = 0; i < sys->nthreads; i++)
		create_thread(bay, i);

	for (size_t i = 0; i < sys->ncpus; i++)
		create_cpu(bay, i);
}

int
ovni_model_create(void *p)
{
	struct emu *emu = emu_get(p);

	/* Get paraver traces */
	//oemu->pvt_thread = pvman_new(emu->pvman, "thread");
	//oemu->pvt_cpu = pvman_new(emu->pvman, "cpu");

	create_channels(&emu->system, &emu->bay);
	return 0;
}
