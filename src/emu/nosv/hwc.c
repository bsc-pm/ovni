#include "hwc.h"

#include "chan.h"
#include "cpu.h"
#include "emu.h"
#include "emu_ev.h"
#include "emu_prv.h"
#include "inttypes.h"
#include "nosv_priv.h"
#include "ovni.h"
#include "parson.h"
#include "proc.h"
#include "pv/cfg_file.h"
#include "pv/pcf.h"
#include "pv/prv.h"
#include "pv/pvt.h"
#include "thread.h"
#include "track.h"
#include "uthash.h"
#include <errno.h>

static int
parse_hwc(struct nosv_hwc_emu *hwc_emu, const char *indexstr, JSON_Value *hwcval)
{
	errno = 0;
	char *endptr = NULL;
	size_t index = (size_t) strtol(indexstr, &endptr, 10);

	if (errno != 0 || endptr == indexstr || endptr[0] != '\0') {
		err("failed to parse hwc index: %s", indexstr);
		return -1;
	}

	if (index >= 100) {
		err("hwc index should be in [0, 100) range: %zd", index);
		return -1;
	}

	if (index >= hwc_emu->n) {
		err("hwc index %zd exceeds allocated counters %zd",
				index, hwc_emu->n);
		return -1;
	}

	JSON_Object *hwc = json_value_get_object(hwcval);
	if (hwc == NULL) {
		err("json_value_get_object() failed");
		return -1;
	}

	const char *name = json_object_get_string(hwc, "name");
	if (name == NULL) {
		err("json_object_get_string() for name failed");
		return -1;
	}

	if (hwc_emu->name[index] == NULL) {
		hwc_emu->name[index] = strdup(name);
		if (hwc_emu->name[index] == NULL) {
			err("strdup failed");
			return -1;
		}
		dbg("got hwc with index %zd and name %s", index, hwc_emu->name[index]);
	} else if (strcmp(hwc_emu->name[index], name) != 0) {
		err("hwc at %zd already defined as %s",
				index, hwc_emu->name[index]);
		return -1;
	}

	return 0;
}

static int
scan_thread(struct nosv_hwc_emu *hwc_emu, struct thread *t)
{
	JSON_Object *obj = json_object_dotget_object(t->meta, "nosv.hwc");

	/* No HWC in this thread */
	if (obj == NULL)
		return 0;

	size_t n = json_object_get_count(obj);

	/* Ignore empty dictionaries */
	if (n == 0)
		return 0;

	if (hwc_emu->n == 0) {
		hwc_emu->name = calloc(n, sizeof(char *));
		hwc_emu->n = n;
	} else if (hwc_emu->n != n) {
		/* We have a mismatch */
		err("thread %s defines %zd hardware counters, but emu already has %zd",
				t->id, n, hwc_emu->n);
		return -1;
	}

	for (size_t i = 0; i < n; i++) {
		const char *indexstr = json_object_get_name(obj, i);
		if (indexstr == NULL) {
			err("json_object_get_name failed");
			return -1;
		}
		JSON_Value *hwcval = json_object_get_value_at(obj, i);
		if (hwcval == NULL) {
			err("json_object_get_value_at failed");
			return -1;
		}

		if (parse_hwc(hwc_emu, indexstr, hwcval) != 0) {
			err("cannot parse HWC");
			return -1;
		}
	}

	return 0;
}

static int
create_thread_chan(struct nosv_hwc_emu *emu, struct bay *bay, struct thread *th)
{
	struct nosv_thread *nosv_thread = EXT(th, 'V');
	struct nosv_hwc_thread *t = &nosv_thread->hwc;

	/* Create as many channels as required */
	t->chan = calloc(emu->n, sizeof(struct chan));
	if (t->chan == NULL) {
		err("calloc failed:");
		return -1;
	}

	t->n = emu->n;

	for (size_t i = 0; i < t->n; i++) {
		struct chan *ch = &t->chan[i];
		chan_init(ch, CHAN_SINGLE, "nosv.thread%"PRIi64".hwc%zd",
				th->gindex, i);

		/* Allow duplicates, we can emit the same HWC value twice */
		chan_prop_set(ch, CHAN_ALLOW_DUP, 1);

		if (bay_register(bay, ch) != 0) {
			err("bay_register failed");
			return -1;
		}
	}

	/* Setup tracking */
	t->track = calloc(t->n, sizeof(struct track));
	if (t->track == NULL) {
		err("calloc failed:");
		return -1;
	}

	for (size_t i = 0; i < t->n; i++) {
		struct track *track = &t->track[i];
		/* For now only tracking to active thread is supported */
		if (track_init(track, bay, TRACK_TYPE_TH, TRACK_TH_ACT,
					"nosv.thread%"PRIi64".hwc%zd",
					th->gindex, i) != 0) {
			err("track_init failed");
			return -1;
		}
	}

	return 0;
}

static int
init_cpu(struct nosv_hwc_emu *emu, struct bay *bay, struct cpu *cpu)
{
	struct nosv_cpu *nosv_cpu = EXT(cpu, 'V');
	struct nosv_hwc_cpu *c = &nosv_cpu->hwc;

	/* Setup tracking */
	c->track = calloc(emu->n, sizeof(struct track));
	if (c->track == NULL) {
		err("calloc failed:");
		return -1;
	}

	c->n = emu->n;

	for (size_t i = 0; i < c->n; i++) {
		struct track *track = &c->track[i];
		/* For now only tracking to running thread is supported */
		if (track_init(track, bay, TRACK_TYPE_TH, TRACK_TH_RUN,
					"nosv.cpu%"PRIi64".hwc%zd",
					cpu->gindex, i) != 0) {
			err("track_init failed");
			return -1;
		}
	}

	return 0;
}

int
hwc_create(struct emu *emu)
{
	struct nosv_emu *nosv_emu = EXT(emu, 'V');
	struct nosv_hwc_emu *hwc_emu = &nosv_emu->hwc;

	for (struct thread *th = emu->system.threads; th; th = th->gnext) {
		if (scan_thread(hwc_emu, th) != 0) {
			err("scan_thread failed");
			return -1;
		}
	}

	/* Early exit if no counters found */
	if (hwc_emu->n == 0) {
		dbg("no hwc counters found");
		return 0;
	}

	/* Create a buffer to do aligned reads */
	hwc_emu->values = calloc(hwc_emu->n, sizeof(int64_t));
	if (hwc_emu->values == NULL) {
		err("calloc failed:");
		return -1;
	}

	/* Once we know how many HWC we have, allocate the channels for threads
	 * and CPUs. */

	for (struct thread *th = emu->system.threads; th; th = th->gnext) {
		if (create_thread_chan(hwc_emu, &emu->bay, th) != 0) {
			err("create_thread_chan failed");
			return -1;
		}
	}

	for (struct cpu *cpu = emu->system.cpus; cpu; cpu = cpu->next) {
		if (init_cpu(hwc_emu, &emu->bay, cpu) != 0) {
			err("init_cpu failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_thread_prv(struct bay *bay, struct thread *systh, struct prv *prv)
{
	struct nosv_thread *nosv_thread = EXT(systh, 'V');
	struct nosv_hwc_thread *t = &nosv_thread->hwc;

	for (size_t i = 0; i < t->n; i++) {
		struct track *track = &t->track[i];
		struct chan *ch = &t->chan[i];
		struct chan *sel = &systh->chan[TH_CHAN_STATE];

		/* Connect the input and sel channel to the mux */
		if (track_connect_thread(track, ch, sel, 1) != 0) {
			err("track_connect_thread failed");
			return -1;
		}

		/* Then connect the output of the tracking module to the prv
		 * trace for the current thread */
		struct chan *out = track_get_output(track);
		long row = (long) systh->gindex;
		long flags = PRV_SKIPDUPNULL | PRV_ZERO;
		long prvtype = (long) (PRV_NOSV_HWC + i);
		if (prv_register(prv, row, prvtype, bay, out, flags)) {
			err("prv_register failed");
			return -1;
		}
	}

	return 0;
}

static int
init_pcf(struct emu *emu, struct pcf *pcf)
{
	struct nosv_emu *nosv_emu = EXT(emu, 'V');
	struct nosv_hwc_emu *hwc_emu = &nosv_emu->hwc;

	for (size_t i = 0; i < hwc_emu->n; i++) {
		long prvtype = (long) (PRV_NOSV_HWC + i);
		const char *name = hwc_emu->name[i];
		struct pcf_type *pcftype = pcf_add_type(pcf, (int) prvtype, name);
		if (pcftype == NULL) {
			err("pcf_add_type failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_thread(struct emu *emu)
{
	struct pvt *pvt = recorder_find_pvt(&emu->recorder, "thread");
	if (pvt == NULL) {
		err("cannot find thread pvt");
		return -1;
	}

	/* Connect thread channels to PRV */
	struct prv *prv = pvt_get_prv(pvt);
	for (struct thread *t = emu->system.threads; t; t = t->gnext) {
		if (connect_thread_prv(&emu->bay, t, prv) != 0) {
			err("connect_thread_prv failed");
			return -1;
		}
	}

	/* Init thread PCF */
	struct pcf *pcf = pvt_get_pcf(pvt);
	if (init_pcf(emu, pcf) != 0) {
		err("init_pcf failed");
		return -1;
	}

	return 0;
}

static int
connect_cpu_prv(struct emu *emu, struct cpu *syscpu, struct prv *prv)
{
	struct nosv_emu *nosv_emu = EXT(emu, 'V');
	struct nosv_hwc_emu *hwc_emu = &nosv_emu->hwc;

	struct nosv_cpu *nosv_cpu = EXT(syscpu, 'V');
	struct nosv_hwc_cpu *hwc_cpu = &nosv_cpu->hwc;

	for (size_t i = 0; i < hwc_emu->n; i++) {
		struct track *track = &hwc_cpu->track[i];
		struct chan *sel = cpu_get_th_chan(syscpu);

		int64_t nthreads = (int64_t) emu->system.nthreads;
		if (track_set_select(track, sel, NULL, nthreads) != 0) {
			err("track_select failed");
			return -1;
		}

		/* Add each thread as input */
		for (struct thread *t = emu->system.threads; t; t = t->gnext) {
			struct nosv_thread *nosv_thread = EXT(t, 'V');
			struct nosv_hwc_thread *hwc_thread = &nosv_thread->hwc;

			/* Use the input thread directly */
			struct chan *inp = &hwc_thread->chan[i];

			if (track_set_input(track, t->gindex, inp) != 0) {
				err("track_add_input failed");
				return -1;
			}
		}

		/* Then connect the output of the tracking module to the prv
		 * trace for the current cpu */
		struct chan *out = track_get_output(track);
		long row = (long) syscpu->gindex;
		long flags = PRV_SKIPDUPNULL | PRV_ZERO;
		long prvtype = (long) (PRV_NOSV_HWC + i);
		if (prv_register(prv, row, prvtype, &emu->bay, out, flags)) {
			err("prv_register failed");
			return -1;
		}
	}

	return 0;
}

static int
connect_cpu(struct emu *emu)
{
	/* Get cpu PRV */
	struct pvt *pvt = recorder_find_pvt(&emu->recorder, "cpu");
	if (pvt == NULL) {
		err("cannot find thread pvt");
		return -1;
	}

	/* Connect cpu channels to PRV */
	struct prv *prv = pvt_get_prv(pvt);
	for (struct cpu *cpu = emu->system.cpus; cpu; cpu = cpu->next) {
		if (connect_cpu_prv(emu, cpu, prv) != 0) {
			err("connect_cpu_prv failed");
			return -1;
		}
	}

	/* Init cpu PCF */
	struct pcf *pcf = pvt_get_pcf(pvt);
	if (init_pcf(emu, pcf) != 0) {
		err("init_pcf failed");
		return -1;
	}

	return 0;
}

/* Connect the channels to the output PVTs */
int
hwc_connect(struct emu *emu)
{
	struct nosv_emu *nosv_emu = EXT(emu, 'V');
	struct nosv_hwc_emu *hwc_emu = &nosv_emu->hwc;

	/* No HWC, so nothing to connect */
	if (hwc_emu->n == 0)
		return 0;

	if (connect_thread(emu) != 0) {
		err("connect_thread() failed");
		return -1;
	}

	if (connect_cpu(emu) != 0) {
		err("connect_cpu() failed");
		return -1;
	}

	return 0;
}

static int
event_hwc_count(struct emu *emu)
{
	struct nosv_emu *nosv_emu = EXT(emu, 'V');
	struct nosv_hwc_emu *hwc_emu = &nosv_emu->hwc;

	if (!emu->ev->is_jumbo) {
		err("expecting a jumbo event");
		return -1;
	}

	/* Make sure size matches */
	size_t array_size = (size_t) emu->ev->payload->jumbo.size;
	size_t expected_size = hwc_emu->n * sizeof(int64_t);
	if (array_size != expected_size) {
		err("unexpected hwc event with jumbo payload size %zd (expecting %zd)",
				array_size, expected_size);
		return -1;
	}

	/* Use memcpy to align array */
	memcpy(hwc_emu->values, &emu->ev->payload->jumbo.data[0], array_size);

	/* Update all HWC channels for the given thread */
	for (size_t i = 0; i < hwc_emu->n; i++) {
		struct nosv_thread *nosv_thread = EXT(emu->thread, 'V');
		struct nosv_hwc_thread *hwc_thread = &nosv_thread->hwc;
		struct chan *ch = &hwc_thread->chan[i];
		if (chan_set(ch, value_int64(hwc_emu->values[i])) != 0) {
			err("chan_set failed for hwc channel %s", ch->name);
			return -1;
		}
	}

	return 0;
}

int
hwc_event(struct emu *emu)
{
	struct nosv_emu *nosv_emu = EXT(emu, 'V');
	struct nosv_hwc_emu *hwc_emu = &nosv_emu->hwc;

	/* Panic on HWC event with no counters */
	if (hwc_emu->n == 0) {
		err("got hwc event %s but no counters enabled", emu->ev->mcv);
		return -1;
	}

	switch (emu->ev->v) {
		case 'C':
			return event_hwc_count(emu);
		default:
			err("unknown nosv hwc event %s", emu->ev->mcv);
			return -1;
	}

	return 0;
}

static int
write_cfg(const char *path, size_t i, const char *fmt, const char *name)
{
	char title[MAX_LABEL];

	/* May truncate silently, but is safe */
	snprintf(title, MAX_LABEL, fmt, name);

	long type = (long) (PRV_NOSV_HWC + i);

	struct cfg_file cf;
	cfg_file_init(&cf, type, title);
	cfg_file_color_mode(&cf, CFG_NGRAD);
	cfg_file_semantic_thread(&cf, CFG_NEXT_EV_VAL);

	if (cfg_file_write(&cf, path) != 0) {
		err("cfg_file_write failed");
		return -1;
	}

	return 0;
}

int
hwc_finish(struct emu *emu)
{
	struct nosv_emu *nosv_emu = EXT(emu, 'V');
	struct nosv_hwc_emu *hwc_emu = &nosv_emu->hwc;

	if (hwc_emu->n == 0)
		return 0;

	/* Write CFG files with HWC names. */

	for (size_t i = 0; i < hwc_emu->n; i++) {
		const char *dir = emu->args.tracedir;
		const char *name = hwc_emu->name[i];
		char path[PATH_MAX];

		/* Create thread configs */
		if (snprintf(path, PATH_MAX, "%s/cfg/thread/nosv/hwc-%s.cfg", dir, name) >= PATH_MAX) {
			err("hwc thread cfg path too long");
			return -1;
		}
		if (write_cfg(path, i, "Thread: nOS-V %s of the ACTIVE thread", name) != 0) {
			err("write_cfg failed");
			return -1;
		}

		/* Same for CPUs */
		if (snprintf(path, PATH_MAX, "%s/cfg/cpu/nosv/hwc-%s.cfg", dir, name) >= PATH_MAX) {
			err("hwc cpu cfg path too long");
			return -1;
		}
		if (write_cfg(path, i, "CPU: nOS-V %s of the RUNNING thread", name) != 0) {
			err("write_cfg failed");
			return -1;
		}
	}

	return 0;
}
