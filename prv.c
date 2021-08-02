#include <stdio.h>
#include <assert.h> 

#include "ovni.h"
#include "emu.h"

void
prv_ev_row(struct ovni_emu *emu, int row, int type, int val)
{
	printf("2:0:1:1:%d:%ld:%d:%d\n",
			row, emu->delta_time, type, val);
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
	row = cpu->i + 1;

	prv_ev_row(emu, row, type, val);
}
