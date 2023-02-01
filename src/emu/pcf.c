/* Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "pcf.h"

#include "common.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>

/* clang-format off */

const char *pcf_def_header =
	"DEFAULT_OPTIONS\n"
	"\n"
	"LEVEL               THREAD\n"
	"UNITS               NANOSEC\n"
	"LOOK_BACK           100\n"
	"SPEED               1\n"
	"FLAG_ICONS          ENABLED\n"
	"NUM_OF_STATE_COLORS 1000\n"
	"YMAX_SCALE          37\n"
	"\n"
	"\n"
	"DEFAULT_SEMANTIC\n"
	"\n"
	"THREAD_FUNC         State As Is\n";

#define RGB(r, g, b) (r<<16 | g<<8 | b)
#define ARRAY_LEN(x)  (sizeof(x) / sizeof((x)[0]))

/* Define colors for the trace */
#define DEEPBLUE  RGB(  0,   0, 255)
#define LIGHTGREY RGB(217, 217, 217)
#define RED       RGB(230,  25,  75)
#define GREEN     RGB(60,  180,  75)
#define YELLOW    RGB(255, 225,  25)
#define ORANGE    RGB(245, 130,  48)
#define PURPLE    RGB(145,  30, 180)
#define CYAN      RGB( 70, 240, 240)
#define MAGENTA   RGB(240, 50,  230)
#define LIME      RGB(210, 245,  60)
#define PINK      RGB(250, 190, 212)
#define TEAL      RGB(  0, 128, 128)
#define LAVENDER  RGB(220, 190, 255)
#define BROWN     RGB(170, 110,  40)
#define BEIGE     RGB(255, 250, 200)
#define MAROON    RGB(128,   0,   0)
#define MINT      RGB(170, 255, 195)
#define OLIVE     RGB(128, 128,   0)
#define APRICOT   RGB(255, 215, 180)
#define NAVY      RGB(  0,   0, 128)
#define BLUE      RGB(  0, 130, 200)
#define GREY      RGB(128, 128, 128)
#define BLACK     RGB(  0,   0,   0)

const uint32_t pcf_def_palette[] = {
	BLACK,		/* (never shown anyways) */
	BLUE,		/* runtime */
	LIGHTGREY,	/* busy wait */
	RED,		/* task */
	GREEN,
	YELLOW, ORANGE, PURPLE, CYAN, MAGENTA, LIME, PINK,
	TEAL, GREY, LAVENDER, BROWN, BEIGE, MAROON, MINT,
	OLIVE, APRICOT, NAVY, DEEPBLUE
};

const uint32_t *pcf_palette = pcf_def_palette;
const int pcf_palette_len = ARRAY_LEN(pcf_def_palette);

///* ------------------ Value labels --------------------- */
//
//struct pcf_value_label default_values[] = {
//	{ ST_TOO_MANY_TH, "Unknown: Multiple threads running" },
//	{ -1, NULL },
//};
//
//struct pcf_value_label ovni_state_values[] = {
//	{ TH_ST_UNKNOWN, "Unknown" },
//	{ TH_ST_RUNNING, "Running" },
//	{ TH_ST_PAUSED,  "Paused"  },
//	{ TH_ST_DEAD,    "Dead"    },
//	{ TH_ST_COOLING, "Cooling" },
//	{ TH_ST_WARMING, "Warming" },
//	{ -1, NULL },
//};
//
//struct pcf_value_label ovni_flush_values[] = {
//	{ 0, "None" },
//	{ ST_OVNI_FLUSHING, "Flushing" },
//	{ ST_TOO_MANY_TH, "Unknown flushing state: Multiple threads running" },
//	{ -1, NULL },
//};
//
//struct pcf_value_label nosv_ss_values[] = {
//	/* Errors */
//	{ ST_BAD,                       "Unknown: bad happened (report bug)" },
//	{ ST_TOO_MANY_TH,               "Unknown: multiple threads running" },
//	/* Good values */
//	{ ST_NULL,                      "No subsystem" },
//	{ ST_NOSV_SCHED_HUNGRY,         "Scheduler: Hungry" },
//	{ ST_NOSV_SCHED_SERVING,        "Scheduler: Serving" },
//	{ ST_NOSV_SCHED_SUBMITTING,     "Scheduler: Submitting" },
//	{ ST_NOSV_MEM_ALLOCATING,       "Memory: Allocating" },
//	{ ST_NOSV_MEM_FREEING,          "Memory: Freeing" },
//	{ ST_NOSV_TASK_RUNNING,         "Task: Running" },
//	{ ST_NOSV_API_SUBMIT,           "API: Submit" },
//	{ ST_NOSV_API_PAUSE,            "API: Pause" },
//	{ ST_NOSV_API_YIELD,            "API: Yield" },
//	{ ST_NOSV_API_WAITFOR,          "API: Waitfor" },
//	{ ST_NOSV_API_SCHEDPOINT,       "API: Scheduling point" },
//	{ ST_NOSV_ATTACH,               "Thread: Attached" },
//	{ ST_NOSV_WORKER,               "Thread: Worker" },
//	{ ST_NOSV_DELEGATE,             "Thread: Delegate" },
//	{ EV_NOSV_SCHED_SEND,           "EV Scheduler: Send task" },
//	{ EV_NOSV_SCHED_RECV,           "EV Scheduler: Recv task" },
//	{ EV_NOSV_SCHED_SELF,           "EV Scheduler: Self-assign task" },
//	{ -1, NULL },
//};
//
//struct pcf_value_label nodes_mode_values[] = {
//	{ ST_NULL,              "NULL" },
//	{ ST_TOO_MANY_TH,       "NODES: Multiple threads running" },
//	{ ST_NODES_REGISTER,    "Dependencies: Registering task accesses" },
//	{ ST_NODES_UNREGISTER,  "Dependencies: Unregistering task accesses" },
//	{ ST_NODES_IF0_WAIT,    "If0: Waiting for an If0 task" },
//	{ ST_NODES_IF0_INLINE,  "If0: Executing an If0 task inline" },
//	{ ST_NODES_TASKWAIT,    "Taskwait: Taskwait" },
//	{ ST_NODES_CREATE,      "Add Task: Creating a task" },
//	{ ST_NODES_SUBMIT,      "Add Task: Submitting a task" },
//	{ ST_NODES_SPAWN,       "Spawn Function: Spawning a function" },
//	{ -1, NULL },
//};
//
//struct pcf_value_label kernel_cs_values[] = {
//	{ ST_NULL,              "NULL" },
//	{ ST_TOO_MANY_TH,       "Unknown: multiple threads running" },
//	{ ST_KERNEL_CSOUT,      "Context switch: Out of the CPU" },
//	{ -1, NULL },
//};
//
//struct pcf_value_label nanos6_ss_values[] = {
//	{ ST_NULL,                    "No subsystem" },
//	{ ST_TOO_MANY_TH,             "Unknown: multiple threads running" },
//	{ ST_NANOS6_TASK_BODY,        "Task: Running body" },
//	{ ST_NANOS6_TASK_CREATING,    "Task: Creating" },
//	{ ST_NANOS6_TASK_SUBMIT,      "Task: Submitting" },
//	{ ST_NANOS6_TASK_SPAWNING,    "Task: Spawning function" },
//	{ ST_NANOS6_TASK_FOR,         "Task: Running task for" },
//	{ ST_NANOS6_SCHED_SERVING,    "Scheduler: Serving tasks" },
//	{ ST_NANOS6_SCHED_ADDING,     "Scheduler: Adding ready tasks" },
//	{ ST_NANOS6_SCHED_PROCESSING, "Scheduler: Processing ready tasks" },
//	{ ST_NANOS6_DEP_REG,          "Dependency: Registering" },
//	{ ST_NANOS6_DEP_UNREG,        "Dependency: Unregistering" },
//	{ ST_NANOS6_BLK_TASKWAIT,     "Blocking: Taskwait" },
//	{ ST_NANOS6_BLK_BLOCKING,     "Blocking: Blocking current task" },
//	{ ST_NANOS6_BLK_UNBLOCKING,   "Blocking: Unblocking remote task" },
//	{ ST_NANOS6_BLK_WAITFOR,      "Blocking: Wait for deadline" },
//	{ ST_NANOS6_HANDLING_TASK,    "Worker: Handling task" },
//	{ ST_NANOS6_WORKER_LOOP,      "Worker: Looking for work" },
//	{ ST_NANOS6_SWITCH_TO,        "Worker: Switching to another thread" },
//	{ ST_NANOS6_MIGRATE,          "Worker: Migrating CPU" },
//	{ ST_NANOS6_SUSPEND,          "Worker: Suspending thread" },
//	{ ST_NANOS6_RESUME,           "Worker: Resuming another thread" },
//	{ ST_NANOS6_ALLOCATING,       "Memory: Allocating" },
//	{ ST_NANOS6_FREEING,          "Memory: Freeing" },
//	{ EV_NANOS6_SCHED_SEND,       "EV Scheduler: Send task" },
//	{ EV_NANOS6_SCHED_RECV,       "EV Scheduler: Recv task" },
//	{ EV_NANOS6_SCHED_SELF,       "EV Scheduler: Self-assign task" },
//	{ EV_NANOS6_CPU_IDLE,         "EV CPU: Becomes idle" },
//	{ EV_NANOS6_CPU_ACTIVE,       "EV CPU: Becomes active" },
//	{ EV_NANOS6_SIGNAL,           "EV Worker: Wakening another thread" },
//	{ -1, NULL },
//};
//
//struct pcf_value_label nanos6_thread_type[] = {
//	{ ST_NULL,                 "No type" },
//	{ ST_TOO_MANY_TH,          "Unknown: multiple threads running" },
//	{ ST_NANOS6_TH_EXTERNAL,   "External" },
//	{ ST_NANOS6_TH_WORKER,     "Worker" },
//	{ ST_NANOS6_TH_LEADER,     "Leader" },
//	{ ST_NANOS6_TH_MAIN,       "Main" },
//	{ -1, NULL },
//};
//
//struct pcf_value_label (*pcf_chan_value_labels[CHAN_MAX])[] = {
//	[CHAN_OVNI_PID]         = &default_values,
//	[CHAN_OVNI_TID]         = &default_values,
//	[CHAN_OVNI_NRTHREADS]   = &default_values,
//	[CHAN_OVNI_STATE]       = &ovni_state_values,
//	[CHAN_OVNI_APPID]       = &default_values,
//	[CHAN_OVNI_CPU]         = &default_values,
//	[CHAN_OVNI_FLUSH]       = &ovni_flush_values,
//
//	[CHAN_NOSV_TASKID]      = &default_values,
//	[CHAN_NOSV_TYPE]        = &default_values,
//	[CHAN_NOSV_APPID]       = &default_values,
//	[CHAN_NOSV_SUBSYSTEM]   = &nosv_ss_values,
//	[CHAN_NOSV_RANK]        = &default_values,
//
//	[CHAN_NODES_SUBSYSTEM]  = &nodes_mode_values,
//
//	[CHAN_NANOS6_TASKID]    = &default_values,
//	[CHAN_NANOS6_TYPE]  	= &default_values,
//	[CHAN_NANOS6_SUBSYSTEM] = &nanos6_ss_values,
//	[CHAN_NANOS6_RANK]      = &default_values,
//	[CHAN_NANOS6_THREAD]    = &nanos6_thread_type,
//
//	[CHAN_KERNEL_CS]        = &kernel_cs_values,
//};
//
///* ------------------ Type labels --------------------- */
//
//char *pcf_chan_name[CHAN_MAX] = {
//	[CHAN_OVNI_PID]         = "PID",
//	[CHAN_OVNI_TID]         = "TID",
//	[CHAN_OVNI_NRTHREADS]   = "Number of RUNNING threads",
//	[CHAN_OVNI_STATE]       = "Execution state",
//	[CHAN_OVNI_APPID]       = "AppID",
//	[CHAN_OVNI_CPU]         = "CPU affinity",
//	[CHAN_OVNI_FLUSH]       = "Flushing state",
//
//	[CHAN_NOSV_TASKID]      = "nOS-V TaskID",
//	[CHAN_NOSV_TYPE]        = "nOS-V task type",
//	[CHAN_NOSV_APPID]       = "nOS-V task AppID",
//	[CHAN_NOSV_SUBSYSTEM]   = "nOS-V subsystem",
//	[CHAN_NOSV_RANK]        = "nOS-V task MPI rank",
//
//	[CHAN_NODES_SUBSYSTEM]  = "NODES subsystem",
//
//	[CHAN_NANOS6_TASKID]    = "Nanos6 task ID",
//	[CHAN_NANOS6_TYPE]      = "Nanos6 task type",
//	[CHAN_NANOS6_SUBSYSTEM] = "Nanos6 subsystem",
//	[CHAN_NANOS6_RANK]      = "Nanos6 task MPI rank",
//	[CHAN_NANOS6_THREAD]    = "Nanos6 thread type",
//
//	[CHAN_KERNEL_CS]        = "Context switches",
//};
//
//enum pcf_suffix { NONE = 0, CUR_TH, RUN_TH, ACT_TH, SUFFIX_MAX };
//
//char *pcf_suffix_name[SUFFIX_MAX] = {
//	[NONE] = "",
//	[CUR_TH] = "of the CURRENT thread",
//	[RUN_TH] = "of the RUNNING thread",
//	[ACT_TH] = "of the ACTIVE thread",
//};
//
//int pcf_chan_suffix[CHAN_MAX][CHAN_MAXTYPE] = {
//	                        /*  Thread  CPU  */
//	[CHAN_OVNI_PID]         = { RUN_TH, RUN_TH },
//	[CHAN_OVNI_TID]         = { RUN_TH, RUN_TH },
//	[CHAN_OVNI_NRTHREADS]   = { NONE,   NONE   },
//	[CHAN_OVNI_STATE]       = { CUR_TH, NONE   },
//	[CHAN_OVNI_APPID]       = { NONE,   RUN_TH },
//	[CHAN_OVNI_CPU]         = { CUR_TH, NONE   },
//	[CHAN_OVNI_FLUSH]       = { CUR_TH, RUN_TH },
//
//	[CHAN_NOSV_TASKID]      = { RUN_TH, RUN_TH },
//	[CHAN_NOSV_TYPE]        = { RUN_TH, RUN_TH },
//	[CHAN_NOSV_APPID]       = { RUN_TH, RUN_TH },
//	[CHAN_NOSV_SUBSYSTEM]   = { ACT_TH, RUN_TH },
//	[CHAN_NOSV_RANK]        = { RUN_TH, RUN_TH },
//
//	[CHAN_NODES_SUBSYSTEM]  = { RUN_TH, RUN_TH },
//
//	[CHAN_NANOS6_TASKID]    = { RUN_TH, RUN_TH },
//	[CHAN_NANOS6_TYPE]      = { RUN_TH, RUN_TH },
//	[CHAN_NANOS6_SUBSYSTEM] = { ACT_TH, RUN_TH },
//	[CHAN_NANOS6_RANK]      = { RUN_TH, RUN_TH },
//	[CHAN_NANOS6_THREAD]    = { RUN_TH, NONE   },
//
//	[CHAN_KERNEL_CS]        = { RUN_TH, ACT_TH },
//};

/* clang-format on */

/* ----------------------------------------------- */

static void
decompose_rgb(uint32_t col, uint8_t *r, uint8_t *g, uint8_t *b)
{
	*r = (col >> 16) & 0xff;
	*g = (col >> 8) & 0xff;
	*b = (col >> 0) & 0xff;
}

static void
write_header(FILE *f)
{
	fprintf(f, "%s", pcf_def_header);
}

static void
write_colors(FILE *f, const uint32_t *palette, int n)
{
	fprintf(f, "\n\n");
	fprintf(f, "STATES_COLOR\n");

	for (int i = 0; i < n; i++) {
		uint8_t r, g, b;
		decompose_rgb(palette[i], &r, &g, &b);
		fprintf(f, "%-3d {%3d, %3d, %3d}\n", i, r, g, b);
	}
}

static void
write_type(FILE *f, struct pcf_type *type)
{
	fprintf(f, "\n\n");
	fprintf(f, "EVENT_TYPE\n");
	fprintf(f, "0 %-10d %s\n", type->id, type->label);
	fprintf(f, "VALUES\n");

	for (struct pcf_value *v = type->values; v != NULL; v = v->hh.next)
		fprintf(f, "%-4d %s\n", v->value, v->label);
}

static void
write_types(struct pcf *pcf)
{
	for (struct pcf_type *t = pcf->types; t != NULL; t = t->hh.next)
		write_type(pcf->f, t);
}

/** Open the given PCF file and create the default events. */
int
pcf_open(struct pcf *pcf, char *path)
{
	memset(pcf, 0, sizeof(*pcf));

	pcf->f = fopen(path, "w");

	if (pcf->f == NULL) {
		err("cannot open PCF file '%s':", path);
		return -1;
	}

	return 0;
}

struct pcf_type *
pcf_find_type(struct pcf *pcf, int type_id)
{
	struct pcf_type *type;

	HASH_FIND_INT(pcf->types, &type_id, type);

	return type;
}

/** Creates a new pcf_type with the given type_id and label. The label
 * can be disposed after return.
 *
 * @return The pcf_type created.
 */
struct pcf_type *
pcf_add_type(struct pcf *pcf, int type_id, const char *label)
{
	struct pcf_type *pcftype = pcf_find_type(pcf, type_id);

	if (pcftype != NULL)
		die("PCF type %d already defined\n", type_id);

	pcftype = calloc(1, sizeof(struct pcf_type));
	if (pcftype == NULL)
		die("calloc failed: %s\n", strerror(errno));

	pcftype->id = type_id;
	pcftype->values = NULL;
	pcftype->nvalues = 0;

	int len = snprintf(pcftype->label, MAX_PCF_LABEL, "%s", label);
	if (len >= MAX_PCF_LABEL)
		die("PCF type label too long\n");

	HASH_ADD_INT(pcf->types, id, pcftype);

	return pcftype;
}

struct pcf_value *
pcf_find_value(struct pcf_type *type, int value)
{
	struct pcf_value *pcfvalue;

	HASH_FIND_INT(type->values, &value, pcfvalue);

	return pcfvalue;
}

/** Adds a new value to the given pcf_type. The label can be disposed
 * after return.
 *
 * @return The new pcf_value created
 */
struct pcf_value *
pcf_add_value(struct pcf_type *type, int value, const char *label)
{
	struct pcf_value *pcfvalue = pcf_find_value(type, value);

	if (pcfvalue != NULL)
		die("PCF value %d already in type %d\n", value, type->id);

	pcfvalue = calloc(1, sizeof(struct pcf_value));
	if (pcfvalue == NULL)
		die("calloc failed: %s\n", strerror(errno));

	pcfvalue->value = value;

	int len = snprintf(pcfvalue->label, MAX_PCF_LABEL, "%s", label);
	if (len >= MAX_PCF_LABEL)
		die("PCF value label too long\n");

	HASH_ADD_INT(type->values, value, pcfvalue);

	type->nvalues++;

	return pcfvalue;
}

/** Writes the defined event and values to the PCF file and closes the file. */
int
pcf_close(struct pcf *pcf)
{
	write_header(pcf->f);
	write_colors(pcf->f, pcf_palette, pcf_palette_len);
	write_types(pcf);

	fclose(pcf->f);
	return 0;
}
