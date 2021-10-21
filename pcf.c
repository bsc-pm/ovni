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

struct event_value thread_state_values[] = {
	{ TH_ST_UNKNOWN, "Unknown" },
	{ TH_ST_RUNNING, "Running" },
	{ TH_ST_PAUSED,  "Paused"  },
	{ TH_ST_DEAD,    "Dead"    },
	{ -1, NULL },
};

struct event_type thread_state = {
	0, PTT_THREAD_STATE, "Thread: State",
	thread_state_values
};

struct event_value thread_tid_values[] = {
	{ 0, "None" },
	{ 1, "Multiple threads" },
	{ -1, NULL },
};

struct event_type thread_tid = {
	0, PTC_THREAD_TID, "CPU: Thread TID",
	thread_tid_values
};

struct event_value ss_values[] = {
	{ ST_NULL, "NULL" },
	{ ST_BAD,		"Unknown subsystem: multiple threads" },
	{ ST_SCHED_HUNGRY,	"Scheduler: Hungry" },
	{ ST_SCHED_SERVING,	"Scheduler: Serving" },
	{ ST_SCHED_SUBMITTING,	"Scheduler: Submitting" },
	{ EV_SCHED_SEND,	"EV Scheduler: Send task" },
	{ EV_SCHED_RECV,	"EV Scheduler: Recv task" },
	{ EV_SCHED_SELF,	"EV Scheduler: Self-assign task" },
	{ ST_MEM_ALLOCATING,	"Memory: Allocating" },
	{ -1, NULL },
};

struct event_type thread_ss = {
	0, PTT_SUBSYSTEM, "Thread: Subsystem",
	ss_values
};

struct event_type cpu_ss = {
	0, PTC_SUBSYSTEM, "CPU: Current thread subsystem",
	ss_values
};

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
	uint32_t col;
	uint8_t r, g, b;

	fprintf(f, "\n\n");
	fprintf(f, "STATES_COLOR\n");

	for(i=0; i<n; i++)
	{
		col = palette[i];
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
write_events(FILE *f)
{
	write_event_type(f, &thread_state);
	write_event_type(f, &thread_tid);
	write_event_type(f, &thread_ss);
	write_event_type(f, &cpu_ss);
}

int
pcf_write(FILE *f)
{
	write_header(f);
	write_colors(f, pcf_palette, pcf_palette_len);
	write_events(f);

	return 0;
}
