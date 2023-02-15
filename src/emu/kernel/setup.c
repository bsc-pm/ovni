#include "kernel_priv.h"

static const char model_name[] = "kernel";
enum { model_id = 'K' };

struct model_spec model_kernel = {
	.name = model_name,
	.model = model_id,
	.create  = kernel_create,
	.connect = kernel_connect,
	.event   = kernel_event,
	.probe   = kernel_probe,
};

/* ----------------- channels ------------------ */

static const char *chan_name[CH_MAX] = {
	[CH_CS] = "subsystem",
};

static const int chan_stack[CH_MAX] = {
	[CH_CS] = 1,
};

/* ----------------- pvt ------------------ */

static const int pvt_type[] = {
	[CH_CS] = 45,
};

static const char *pcf_prefix[CH_MAX] = {
	[CH_CS]   = "Kernel subsystem",
};

static const struct pcf_value_label kernel_cs_values[] = {
	{ ST_CSOUT,      "Context switch: Out of the CPU" },
	{ -1, NULL },
};

static const struct pcf_value_label (*pcf_labels[CH_MAX])[] = {
	[CH_CS] = &kernel_cs_values,
};

static const struct model_pvt_spec pvt_spec = {
	.type = pvt_type,
	.prefix = pcf_prefix,
	.label = pcf_labels,
};

/* ----------------- tracking ------------------ */

static const int th_track[CH_MAX] = {
	[CH_CS] = TRACK_TH_ANY,
};

static const int cpu_track[CH_MAX] = {
	[CH_CS] = TRACK_TH_RUN, /* FIXME: Why active */
};

/* ----------------- chan_spec ------------------ */

static const struct model_chan_spec th_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.pvt = &pvt_spec,
	.track = th_track,
};

static const struct model_chan_spec cpu_chan = {
	.nch = CH_MAX,
	.prefix = model_name,
	.ch_names = chan_name,
	.ch_stack = chan_stack,
	.pvt = &pvt_spec,
	.track = cpu_track,
};

/* ----------------- models ------------------ */

static const struct model_cpu_spec cpu_spec = {
	.size = sizeof(struct kernel_cpu),
	.chan = &cpu_chan,
	.model = &model_kernel,
};

static const struct model_thread_spec th_spec = {
	.size = sizeof(struct kernel_thread),
	.chan = &th_chan,
	.model = &model_kernel,
};

/* ----------------------------------------------------- */

int
kernel_probe(struct emu *emu)
{
	if (emu->system.nthreads == 0)
		return 1;

	return 0;
}

int
kernel_create(struct emu *emu)
{
	if (model_thread_create(emu, &th_spec) != 0) {
		err("model_thread_init failed");
		return -1;
	}

	if (model_cpu_create(emu, &cpu_spec) != 0) {
		err("model_cpu_init failed");
		return -1;
	}

	return 0;
}

int
kernel_connect(struct emu *emu)
{
	if (model_thread_connect(emu, &th_spec) != 0) {
		err("model_thread_connect failed");
		return -1;
	}

	if (model_cpu_connect(emu, &cpu_spec) != 0) {
		err("model_cpu_connect failed");
		return -1;
	}

	return 0;
}