#include "ovni.h"
#include "emu.h"

#include <assert.h>

struct ovni_cpu *
emu_get_cpu(struct ovni_emu *emu, int cpuid)
{
	assert(cpuid < OVNI_MAX_CPU);

	if(cpuid < 0)
	{
		return &emu->vcpu;
	}

	return &emu->cpu[emu->cpuind[cpuid]];
}

int
emu_cpu_find_thread(struct ovni_cpu *cpu, struct ovni_ethread *thread)
{
	int i;

	for(i=0; i<cpu->nthreads; i++)
		if(cpu->thread[i] == thread)
			break;

	/* Not found */
	if(i >= cpu->nthreads)
		return -1;

	return i;
}

void
emu_cpu_add_thread(struct ovni_cpu *cpu, struct ovni_ethread *thread)
{
	/* Found, abort */
	if(emu_cpu_find_thread(cpu, thread) >= 0)
		abort();

	assert(cpu->nthreads < OVNI_MAX_THR);

	cpu->thread[cpu->nthreads++] = thread;
}

void
emu_cpu_remove_thread(struct ovni_cpu *cpu, struct ovni_ethread *thread)
{
	int i, j;

	i = emu_cpu_find_thread(cpu, thread);

	/* Not found, abort */
	if(i < 0)
		abort();

	for(j=i; j+1 < cpu->nthreads; j++)
	{
		cpu->thread[i] = cpu->thread[j+1];
	}

	cpu->nthreads--;
}


static void
print_threads_state(struct ovni_emu *emu)
{
	struct ovni_cpu *cpu;
	int i, j;

	for(i=0; i<emu->ncpus; i++)
	{
		cpu = &emu->cpu[i];

		dbg("-- cpu %d runs %d threads:", i, cpu->nthreads);
		for(j=0; j<cpu->nthreads; j++)
		{
			dbg(" %d", cpu->thread[j]->tid);
		}
		dbg("\n");
	}

	dbg("-- vcpu runs %d threads:", emu->vcpu.nthreads);
	for(j=0; j<emu->vcpu.nthreads; j++)
	{
		dbg(" %d", emu->vcpu.thread[j]->tid);
	}
	dbg("\n");
}

static void
ev_thread_execute(struct ovni_emu *emu)
{
	struct ovni_cpu *cpu;
	int cpuid;

	/* The thread cannot be already running */
	assert(emu->cur_thread->state != TH_ST_RUNNING);

	cpuid = emu->cur_ev->payload.i32[0];
	dbg("thread %d runs in cpuid %d\n", emu->cur_thread->tid,
			cpuid);
	cpu = emu_get_cpu(emu, cpuid);

	emu->cur_thread->state = TH_ST_RUNNING;
	emu->cur_thread->cpu = cpu;

	emu_cpu_add_thread(cpu, emu->cur_thread);
}

static void
ev_thread_end(struct ovni_emu *emu)
{
	assert(emu->cur_thread->state == TH_ST_RUNNING);
	assert(emu->cur_thread->cpu);

	emu_cpu_remove_thread(emu->cur_thread->cpu, emu->cur_thread);

	emu->cur_thread->state = TH_ST_DEAD;
	emu->cur_thread->cpu = NULL;
}

static void
ev_thread_pause(struct ovni_emu *emu)
{
	assert(emu->cur_thread->state == TH_ST_RUNNING);
	assert(emu->cur_thread->cpu);

	emu_cpu_remove_thread(emu->cur_thread->cpu, emu->cur_thread);

	emu->cur_thread->state = TH_ST_PAUSED;
}

static void
ev_thread_resume(struct ovni_emu *emu)
{
	assert(emu->cur_thread->state == TH_ST_PAUSED);
	assert(emu->cur_thread->cpu);

	emu_cpu_add_thread(emu->cur_thread->cpu, emu->cur_thread);

	emu->cur_thread->state = TH_ST_RUNNING;
}

static void
ev_thread(struct ovni_emu *emu)
{
	struct ovni_ev *ev;
	struct ovni_cpu *cpu;
	struct ovni_ethread *thread, *remote_thread;
	int i;

	emu_emit(emu);

	thread = emu->cur_thread;
	cpu = thread->cpu;
	ev = emu->cur_ev;

	switch(ev->value)
	{
		case 'c': /* create */
			dbg("thread %d creates a new thread at cpu=%d with args=%x %x\n",
					thread->tid,
					ev->payload.u32[0],
					ev->payload.u32[1],
					ev->payload.u32[2]);

			break;
		case 'x': ev_thread_execute(emu); break;
		case 'e': ev_thread_end(emu); break;
		case 'p': ev_thread_pause(emu); break;
		case 'r': ev_thread_resume(emu); break;
		default:
			break;
	}
}

static void
ev_affinity_set(struct ovni_emu *emu)
{
	int cpuid;
	struct ovni_cpu *newcpu;

	cpuid = emu->cur_ev->payload.i32[0];

	assert(emu->cur_thread->state == TH_ST_RUNNING);
	assert(emu->cur_thread->cpu);

	/* Migrate current cpu to the one at cpuid */
	newcpu = emu_get_cpu(emu, cpuid);

	emu_cpu_remove_thread(emu->cur_thread->cpu, emu->cur_thread);
	emu_cpu_add_thread(newcpu, emu->cur_thread);

	emu->cur_thread->cpu = newcpu;

	dbg("cpu %d now runs %d\n", cpuid, emu->cur_thread->tid);
}

static void
ev_affinity_remote(struct ovni_emu *emu)
{
	int cpuid, tid;
	struct ovni_cpu *newcpu;
	struct ovni_ethread *thread;

	cpuid = emu->cur_ev->payload.i32[0];
	tid = emu->cur_ev->payload.i32[1];

	thread = emu_get_thread(emu, tid);

	assert(thread);
	assert(thread->state == TH_ST_PAUSED);
	assert(thread->cpu);

	newcpu = emu_get_cpu(emu, cpuid);

	/* It must not be running in any of the cpus */
	assert(emu_cpu_find_thread(thread->cpu, thread) == -1);
	assert(emu_cpu_find_thread(newcpu, thread) == -1);

	thread->cpu = newcpu;

	dbg("thread %d switches to cpu %d by remote petition\n", tid,
			cpuid);
}

static void
ev_affinity(struct ovni_emu *emu)
{
	emu_emit(emu);
	switch(emu->cur_ev->value)
	{
		case 's': ev_affinity_set(emu); break;
		case 'r': ev_affinity_remote(emu); break;
		default:
			dbg("unknown affinity event value %c\n",
					emu->cur_ev->value);
			break;
	}
}

static void
ev_cpu_count(struct ovni_emu *emu)
{
	int i, ncpus, maxcpu;

	ncpus = emu->cur_ev->payload.i32[0];
	maxcpu = emu->cur_ev->payload.i32[1];

	assert(ncpus < OVNI_MAX_CPU);
	assert(maxcpu < OVNI_MAX_CPU);

	for(i=0; i<OVNI_MAX_CPU; i++)
	{
		emu->cpu[i].state = CPU_ST_UNKNOWN;
		emu->cpu[i].cpu_id = -1;
		emu->cpuind[i] = -1;
	}

	emu->ncpus = 0;
	emu->max_ncpus = ncpus;
}

static void
ev_cpu_id(struct ovni_emu *emu)
{
	int cpuid;

	cpuid = emu->cur_ev->payload.i32[0];

	assert(cpuid < emu->max_ncpus);
	assert(emu->ncpus < emu->max_ncpus);
	assert(emu->ncpus < OVNI_MAX_CPU);

	assert(emu->cpu[emu->ncpus].state == CPU_ST_UNKNOWN);

	emu->cpu[emu->ncpus].state = CPU_ST_READY;
	emu->cpu[emu->ncpus].cpu_id = cpuid;
	emu->cpu[emu->ncpus].index = emu->ncpus;

	/* Fill the translation to cpu index too */
	assert(emu->cpuind[cpuid] == -1);
	emu->cpuind[cpuid] = emu->ncpus;

	dbg("new cpu id=%d at %d\n", cpuid, emu->ncpus);

	emu->ncpus++;
}


static void
ev_cpu(struct ovni_emu *emu)
{
	switch(emu->cur_ev->value)
	{
		case 'n': ev_cpu_count(emu); break;
		case 'i': ev_cpu_id(emu); break;
		default:
			dbg("unknown cpu event value %c\n",
					emu->cur_ev->value);
			break;
	}
}

void
hook_pre_ovni(struct ovni_emu *emu)
{
	//emu_emit(emu);

	switch(emu->cur_ev->class)
	{
		case 'H': ev_thread(emu); break;
		case 'A': ev_affinity(emu); break;
		case 'C': ev_cpu(emu); break;
		case 'B': dbg("burst %c\n", emu->cur_ev->value); break;
		default:
			dbg("unknown ovni event class %c\n",
					emu->cur_ev->class);
			break;
	}

	print_threads_state(emu);
}

static void
view_thread_count(struct ovni_emu *emu)
{
	int i;

	/* Check every CPU looking for a change in nthreads */
	for(i=0; i<emu->ncpus; i++)
	{
		if(emu->cpu[i].last_nthreads == emu->cpu[i].nthreads)
			continue;

		/* Emit the number of threads in the cpu */
		dbg("cpu %d runs %d threads\n", i, emu->cpu[i].nthreads);
	}

	/* Same with the virtual CPU */
	if(emu->vcpu.last_nthreads != emu->vcpu.nthreads)
		dbg("vpu runs %d threads\n", emu->vcpu.nthreads);
}

void
hook_view_ovni(struct ovni_emu *emu)
{
	switch(emu->cur_ev->class)
	{
		case 'H':
		case 'A':
		case 'C':
			view_thread_count(emu);
			break;
		default:
			break;
	}
}

void
hook_post_ovni(struct ovni_emu *emu)
{
	int i;

	/* Update last_nthreads in the CPUs */

	for(i=0; i<emu->ncpus; i++)
		emu->cpu[i].last_nthreads = emu->cpu[i].nthreads;

	emu->vcpu.last_nthreads = emu->vcpu.nthreads;
}

