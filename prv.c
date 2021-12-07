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

#include <stdio.h>

#include "ovni.h"
#include "emu.h"
#include "prv.h"

void
prv_ev(FILE *f, int row, int64_t time, int type, int val)
{
	dbg("<<< 2:0:1:1:%d:%ld:%d:%d\n", row, time, type, val);
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

static void
prv_ev_cpu_raw(struct ovni_emu *emu, int row, int64_t time, int type, int val)
{
	prv_ev(emu->prv_cpu, row, time, type, val);
}

void
prv_ev_cpu(struct ovni_emu *emu, int row, int type, int val)
{
	prv_ev_cpu_raw(emu, row, emu->delta_time, type, val);
}

void
prv_ev_autocpu_raw(struct ovni_emu *emu, int64_t time, int type, int val)
{
	if(emu->cur_thread == NULL)
		die("prv_ev_autocpu_raw: current thread is NULL\n");

	struct ovni_cpu *cpu = emu->cur_thread->cpu;

	if(cpu == NULL)
		die("prv_ev_autocpu_raw: current thread CPU is NULL\n");

	/* FIXME: Use the global index of the CPUs */
	if(cpu->i < 0)
		die("prv_ev_autocpu_raw: CPU index is negative\n");

	/* Begin at 1 */
	int row = emu->cur_loom->offset_ncpus + cpu->i + 1;

	prv_ev_cpu_raw(emu, row, time, type, val);
}

void
prv_ev_autocpu(struct ovni_emu *emu, int type, int val)
{
	prv_ev_autocpu_raw(emu, emu->delta_time, type, val);
}

void
prv_header(FILE *f, int nrows)
{
	fprintf(f, "#Paraver (19/01/38 at 03:14):%020ld_ns:0:1:1(%d:1)\n", 0LU, nrows);
}

void
prv_fix_header(FILE *f, uint64_t duration, int nrows)
{
	/* Go to the first byte */
	fseek(f, 0, SEEK_SET);
	fprintf(f, "#Paraver (19/01/38 at 03:14):%020ld_ns:0:1:1(%d:1)\n", duration, nrows);
}
