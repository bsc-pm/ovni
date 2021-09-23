#include <stdio.h>
#include <assert.h> 

#include "ovni.h"
#include "emu.h"
#include "prv.h"

void
prv_ev(FILE *f, int row, int64_t time, int type, int val)
{
	fprintf(f, "2:0:1:1:%d:%ld:%d:%d\n", row, time, type, val);
}

void
prv_ev_thread_raw(struct ovni_emu *emu, int row, int64_t time, int type, int val)
{
	prv_ev(emu->prv_thread, row, time, type, val);
}

void
prv_ev_thread(struct ovni_emu *emu, int row, int type, int val)
{
	prv_ev_thread_raw(emu, row, emu->delta_time, type, val);
}

void
prv_ev_cpu(struct ovni_emu *emu, int row, int type, int val)
{
	prv_ev(emu->prv_cpu, row, emu->delta_time, type, val);
}

void
prv_ev_autocpu(struct ovni_emu *emu, int type, int val)
{
	int row;
	struct ovni_cpu *cpu;

	assert(emu->cur_thread);

	cpu = emu->cur_thread->cpu;

	assert(cpu);
	assert(cpu->i >= 0);

	/* Begin at 1 */
	row = emu->cur_loom->offset_ncpus + cpu->i + 1;

	prv_ev_cpu(emu, row, type, val);
}

void
prv_header(FILE *f, int nrows)
{
	fprintf(f, "#Paraver (19/01/38 at 03:14):00000000000000000000_ns:0:1:1(%d:1)\n", nrows);
}
