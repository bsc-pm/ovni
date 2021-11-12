/*
 * Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "pcf.h"
#include "prv.h"
#include "emu.h"

#include <stdio.h>
#include <stdint.h>

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
	YELLOW,
	ORANGE,
	PURPLE,
	CYAN,
	MAGENTA,
	LIME,
	PINK,
	TEAL,
	GREY,
	LAVENDER,
	BROWN,
	BEIGE,
	MAROON,
	MINT,
	OLIVE,
	APRICOT,
	NAVY,
	DEEPBLUE
};

const uint32_t *pcf_palette = pcf_def_palette;
const int pcf_palette_len = ARRAY_LEN(pcf_def_palette);

struct event_value {
	int value;
	const char *label;
};

struct event_type {
	int index;
	int type;
	const char *label;
	struct event_value *values;
};

/* ---------------- CHAN_OVNI_PID ---------------- */

struct event_value ovni_pid_values[] = {
	{ 0, "None" },
	{ ST_TOO_MANY_TH, "Unknown PID: Multiple threads running" },
	/* FIXME: PID values may collide with error code values */
	{ -1, NULL },
};

struct event_type thread_ovni_pid = {
	0, chan_to_prvtype[CHAN_OVNI_PID][CHAN_TH],
	"Thread: PID of the RUNNING thread",
	ovni_pid_values
};

struct event_type cpu_ovni_pid = {
	0, chan_to_prvtype[CHAN_OVNI_PID][CHAN_CPU],
	"CPU: PID of the RUNNING thread",
	ovni_pid_values
};

/* ---------------- CHAN_OVNI_TID ---------------- */

struct event_value ovni_tid_values[] = {
	{ 0, "None" },
	{ ST_TOO_MANY_TH, "Unknown TID: Multiple threads running" },
	/* FIXME: TID values may collide with error code values */
	{ -1, NULL },
};

struct event_type thread_ovni_tid = {
	0, chan_to_prvtype[CHAN_OVNI_TID][CHAN_TH],
	"Thread: TID of the RUNNING thread",
	ovni_tid_values
};

struct event_type cpu_ovni_tid = {
	0, chan_to_prvtype[CHAN_OVNI_TID][CHAN_CPU],
	"CPU: TID of the RUNNING thread",
	ovni_tid_values
};

/* ---------------- CHAN_OVNI_NRTHREADS ---------------- */

struct event_value ovni_nthreads_values[] = {
	{ -1, NULL },
};

struct event_type cpu_ovni_nthreads = {
	0, chan_to_prvtype[CHAN_OVNI_NRTHREADS][CHAN_CPU],
	"CPU: Number of RUNNING threads",
	ovni_nthreads_values
};

/* ---------------- CHAN_OVNI_STATE ---------------- */

struct event_value ovni_state_values[] = {
	{ TH_ST_UNKNOWN, "Unknown" },
	{ TH_ST_RUNNING, "Running" },
	{ TH_ST_PAUSED,  "Paused"  },
	{ TH_ST_DEAD,    "Dead"    },
	{ TH_ST_COOLING, "Cooling" },
	{ TH_ST_WARMING, "Warming" },
	{ -1, NULL },
};

struct event_type thread_ovni_state = {
	0, chan_to_prvtype[CHAN_OVNI_STATE][CHAN_TH],
	"Thread: State of the CURRENT thread",
	ovni_state_values
};

/* PRV CPU not used for the state */

/* ---------------- CHAN_OVNI_APPID ---------------- */

/* Not used */

/* ---------------- CHAN_OVNI_CPU ---------------- */

struct event_type thread_cpu_affinity = {
	0, chan_to_prvtype[CHAN_OVNI_CPU][CHAN_TH],
	"Thread: CPU affinity of the CURRENT thread",
	/* Ignored */ NULL
};

/* ---------------- CHAN_OVNI_FLUSH ---------------- */

struct event_value ovni_flush_values[] = {
	{ 0, "None" },
	{ ST_OVNI_FLUSHING, "Flushing" },
	{ ST_TOO_MANY_TH, "Unknown flushing state: Multiple threads running" },
	{ -1, NULL },
};

struct event_type thread_ovni_flush = {
	0, chan_to_prvtype[CHAN_OVNI_FLUSH][CHAN_TH],
	"Thread: Flushing state of the CURRENT thread",
	ovni_flush_values
};

struct event_type cpu_ovni_flush = {
	0, chan_to_prvtype[CHAN_OVNI_FLUSH][CHAN_CPU],
	"CPU: Flusing state of the RUNNING thread",
	ovni_flush_values
};

/* ---------------- CHAN_NOSV_TASKID  ---------------- */

struct event_value nosv_taskid_values[] = {
	{ ST_TOO_MANY_TH, "Unknown TaskID: Multiple threads running" },
	/* FIXME: Task ID values may collide with error code values */
	{ -1, NULL },
};

struct event_type thread_nosv_taskid = {
	0, chan_to_prvtype[CHAN_NOSV_TASKID][CHAN_TH],
	"Thread: nOS-V TaskID of the RUNNING thread",
	nosv_taskid_values
};

struct event_type cpu_nosv_taskid = {
	0, chan_to_prvtype[CHAN_NOSV_TASKID][CHAN_CPU],
	"CPU: nOS-V TaskID of the RUNNING thread",
	nosv_taskid_values
};

/* ---------------- CHAN_NOSV_TYPEID  ---------------- */

struct event_value nosv_typeid_values[] = {
	{ ST_TOO_MANY_TH, "Unknown Task TypeID: Multiple threads running" },
	/* FIXME: Task ID values may collide with error code values */
	{ -1, NULL },
};

struct event_type thread_nosv_typeid = {
	0, chan_to_prvtype[CHAN_NOSV_TYPEID][CHAN_TH],
	"Thread: nOS-V task TypeID of the RUNNING thread",
	nosv_typeid_values
};

struct event_type cpu_nosv_typeid = {
	0, chan_to_prvtype[CHAN_NOSV_TYPEID][CHAN_CPU],
	"CPU: nOS-V task TypeID of the RUNNING thread",
	nosv_typeid_values
};

/* ---------------- CHAN_NOSV_APPID  ---------------- */

struct event_value nosv_appid_values[] = {
	{ ST_TOO_MANY_TH, "Unknown Task AppID: Multiple threads running" },
	/* FIXME: Task ID values may collide with error code values */
	{ -1, NULL },
};

struct event_type thread_nosv_appid = {
	0, chan_to_prvtype[CHAN_NOSV_APPID][CHAN_TH],
	"Thread: nOS-V task AppID of the RUNNING thread",
	nosv_appid_values
};

struct event_type cpu_nosv_appid = {
	0, chan_to_prvtype[CHAN_NOSV_APPID][CHAN_CPU],
	"CPU: nOS-V task AppID of the RUNNING thread",
	nosv_appid_values
};

/* ---------------- CHAN_NOSV_SUBSYSTEM ---------------- */

struct event_value nosv_ss_values[] = {
	/* Errors */
	{ ST_BAD,                       "Unknown subsystem: Bad happened (report bug)" },
	{ ST_TOO_MANY_TH,		"Unknown subsystem: Multiple threads running" },
	/* Good values */
	{ ST_NULL,			"No subsystem" },
	{ ST_NOSV_SCHED_HUNGRY,         "Scheduler: Hungry" },
	{ ST_NOSV_SCHED_SERVING,        "Scheduler: Serving" },
	{ ST_NOSV_SCHED_SUBMITTING,     "Scheduler: Submitting" },
	{ ST_NOSV_TASK_RUNNING,         "Task: Running" },
	{ ST_NOSV_MEM_ALLOCATING,       "Memory: Allocating" },
	{ ST_NOSV_MEM_FREEING,          "Memory: Freeing" },
	{ ST_NOSV_API_SUBMIT,           "API: Submit" },
	{ ST_NOSV_API_PAUSE,            "API: Pause" },
	{ ST_NOSV_API_YIELD,            "API: Yield" },
	{ ST_NOSV_API_WAITFOR,          "API: Waitfor" },
	{ ST_NOSV_API_SCHEDPOINT,       "API: Scheduling point" },
	{ ST_NOSV_ATTACH,               "Thread: Attached" },
	{ ST_NOSV_WORKER,               "Thread: Worker" },
	{ ST_NOSV_DELEGATE,             "Thread: Delegate" },
	{ EV_NOSV_SCHED_SEND,           "EV Scheduler: Send task" },
	{ EV_NOSV_SCHED_RECV,           "EV Scheduler: Recv task" },
	{ EV_NOSV_SCHED_SELF,           "EV Scheduler: Self-assign task" },
	{ -1, NULL },
};

struct event_type thread_nosv_ss = {
	0, chan_to_prvtype[CHAN_NOSV_SUBSYSTEM][CHAN_TH],
	"Thread: nOS-V subsystem of the ACTIVE thread",
	nosv_ss_values
};

struct event_type cpu_nosv_ss = {
	0, chan_to_prvtype[CHAN_NOSV_SUBSYSTEM][CHAN_CPU],
	"CPU: nOS-V subsystem of the RUNNING thread",
	nosv_ss_values
};

/* ---------------- CHAN_TAMPI_MODE ---------------- */

struct event_value tampi_mode_values[] = {
	{ ST_NULL,		"NULL" },
	{ ST_TOO_MANY_TH,	"TAMPI: Unknown, multiple threads running" },
	{ ST_TAMPI_SEND,	"TAMPI: Send" },
	{ ST_TAMPI_RECV,	"TAMPI: Recv" },
	{ ST_TAMPI_ISEND,	"TAMPI: Isend" },
	{ ST_TAMPI_IRECV,	"TAMPI: Irecv" },
	{ ST_TAMPI_WAIT,	"TAMPI: Wait" },
	{ ST_TAMPI_WAITALL,	"TAMPI: Waitall" },
	{ -1, NULL },
};

struct event_type cpu_tampi_mode = {
	0, chan_to_prvtype[CHAN_TAMPI_MODE][CHAN_CPU],
	"CPU: TAMPI mode of the RUNNING thread",
	tampi_mode_values
};

struct event_type thread_tampi_mode = {
	0, chan_to_prvtype[CHAN_TAMPI_MODE][CHAN_TH],
	"Thread: TAMPI mode of the RUNNING thread",
	tampi_mode_values
};

/* ---------------- CHAN_OPENMP_MODE ---------------- */

struct event_value openmp_mode_values[] = {
	{ ST_NULL,		"NULL" },
	{ ST_TOO_MANY_TH,	"OpenMP: Unknown, multiple threads running" },
	{ ST_OPENMP_TASK,	"OpenMP: Task" },
	{ ST_OPENMP_PARALLEL,	"OpenMP: Parallel" },
	{ -1, NULL },
};

struct event_type cpu_openmp_mode = {
	0, chan_to_prvtype[CHAN_OPENMP_MODE][CHAN_CPU],
	"CPU: OpenMP mode of the RUNNING thread",
	openmp_mode_values
};

struct event_type thread_openmp_mode = {
	0, chan_to_prvtype[CHAN_OPENMP_MODE][CHAN_TH],
	"Thread: OpenMP mode of the RUNNING thread",
	openmp_mode_values
};

/* ---------------- CHAN_NANOS6_SUBSYSTEM ---------------- */

struct event_value nanos6_mode_values[] = {
	{ ST_NULL,                 "NULL" },
	{ ST_TOO_MANY_TH,          "Nanos6: Multiple threads running" },
	{ ST_NANOS6_REGISTERING,   "Dependencies: Register task accesses" },
	{ ST_NANOS6_UNREGISTERING, "Dependencies: Unregister task accesses" },
	{ ST_NANOS6_IF0_WAIT,      "If0: Wait for If0 task" },
	{ ST_NANOS6_IF0_INLINE,    "If0: Execute If0 task inline" },
	{ ST_NANOS6_TASKWAIT,      "Taskwait: Taskwait" },
	{ ST_NANOS6_CREATING,      "Add Task: Create a task" },
	{ ST_NANOS6_SUBMITTING,    "Add Task: Submit a task" },
	{ ST_NANOS6_SPAWN,         "Spawn Function: Spawn a function" },
	{ -1, NULL },
};

struct event_type cpu_nanos6_mode = {
	0, chan_to_prvtype[CHAN_NANOS6_SUBSYSTEM][CHAN_CPU],
	"CPU: Nanos6 mode of the RUNNING thread",
	nanos6_mode_values
};

struct event_type thread_nanos6_mode = {
	0, chan_to_prvtype[CHAN_NANOS6_SUBSYSTEM][CHAN_TH],
	"Thread: Nanos6 mode of the RUNNING thread",
	nanos6_mode_values
};

/* ----------------------------------------------- */

static void
decompose_rgb(uint32_t col, uint8_t *r, uint8_t *g, uint8_t *b)
{
	*r = (col>>16) & 0xff;
	*g = (col>>8) & 0xff;
	*b = (col>>0) & 0xff;
}

static void
write_header(FILE *f)
{
	fprintf(f, "%s", pcf_def_header);
}

static void
write_colors(FILE *f, const uint32_t *palette, int n)
{
	int i;
	uint8_t r, g, b;

	fprintf(f, "\n\n");
	fprintf(f, "STATES_COLOR\n");

	for(i=0; i<n; i++)
	{
		decompose_rgb(palette[i], &r, &g, &b);
		fprintf(f, "%-3d {%3d, %3d, %3d}\n", i, r, g, b);
	}
}

static void
write_event_type_header(FILE *f, int index, int type, const char *label)
{
	fprintf(f, "\n\n");
	fprintf(f, "EVENT_TYPE\n");
	fprintf(f, "%-4d %-10d %s\n", index, type, label);
}

static void
write_event_type(FILE *f, struct event_type *ev)
{
	int i;

	write_event_type_header(f, ev->index, ev->type, ev->label);

	fprintf(f, "VALUES\n");

	for(i=0; ev->values[i].label; i++)
	{
		fprintf(f, "%-4d %s\n",
				ev->values[i].value,
				ev->values[i].label);
	}
}

static void
write_cpu_type(FILE *f, struct event_type *ev, struct ovni_emu *emu)
{
	size_t i;

	write_event_type_header(f, ev->index, ev->type, ev->label);

	fprintf(f, "VALUES\n");

	for(i=0; i<emu->total_ncpus; i++)
	{
		fprintf(f, "%-4ld %s\n", i+1, emu->global_cpu[i]->name);
	}
}

static void
write_events(FILE *f, struct ovni_emu *emu)
{
	/* Threads */
	write_event_type(f, &thread_ovni_pid);
	write_event_type(f, &thread_ovni_tid);
	/* thread_ovni_nthreads not needed */
	write_event_type(f, &thread_ovni_state);
	/* thread_ovni_appid not needed */
	write_event_type(f, &thread_ovni_flush);
	write_event_type(f, &thread_nosv_taskid);
	write_event_type(f, &thread_nosv_typeid);
	write_event_type(f, &thread_nosv_appid);
	write_event_type(f, &thread_nosv_ss);
	write_event_type(f, &thread_tampi_mode);
	write_event_type(f, &thread_openmp_mode);
	write_event_type(f, &thread_nanos6_mode);

	/* CPU */
	write_event_type(f, &cpu_ovni_pid);
	write_event_type(f, &cpu_ovni_tid);
	/* cpu_ovni_nthreads not needed */
	/* cpu_ovni_state not needed */
	/* cpu_ovni_appid not needed */
	/* cpu_ovni_cpu not needed */
	write_event_type(f, &cpu_ovni_flush);
	write_event_type(f, &cpu_nosv_taskid);
	write_event_type(f, &cpu_nosv_typeid);
	write_event_type(f, &cpu_nosv_appid);
	write_event_type(f, &cpu_nosv_ss);
	write_event_type(f, &cpu_tampi_mode);
	write_event_type(f, &cpu_openmp_mode);
	write_event_type(f, &cpu_nanos6_mode);

	/* Custom */
	write_cpu_type(f, &thread_cpu_affinity, emu);
}

int
pcf_write(FILE *f, struct ovni_emu *emu)
{
	write_header(f);
	write_colors(f, pcf_palette, pcf_palette_len);
	write_events(f, emu);

	return 0;
}
