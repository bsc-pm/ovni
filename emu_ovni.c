#include "ovni.h"
#include "emu.h"
#include "prv.h"

#include <assert.h>

struct ovni_cpu *
emu_get_cpu(struct ovni_loom *loom, int cpuid)
{
	assert(cpuid < OVNI_MAX_CPU);

	if(cpuid < 0)
	{
		return &loom->vcpu;
	}

	return &loom->cpu[cpuid];
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
print_threads_state(struct ovni_loom *loom)
{
	struct ovni_cpu *cpu;
	int i, j;

	for(i=0; i<loom->ncpus; i++)
	{
		cpu = &loom->cpu[i];

		dbg("-- cpu %d runs %d threads:", i, cpu->nthreads);
		for(j=0; j<cpu->nthreads; j++)
		{
			dbg(" %d", cpu->thread[j]->tid);
		}
		dbg("\n");
	}

	dbg("-- vcpu runs %d threads:", loom->vcpu.nthreads);
	for(j=0; j<loom->vcpu.nthreads; j++)
	{
		dbg(" %d", loom->vcpu.thread[j]->tid);
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
	//dbg("thread %d runs in cpuid %d\n", emu->cur_thread->tid,
	//		cpuid);
	cpu = emu_get_cpu(emu->cur_loom, cpuid);

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

	//emu_emit(emu);

	thread = emu->cur_thread;
	cpu = thread->cpu;
	ev = emu->cur_ev;

	switch(ev->header.value)
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
	newcpu = emu_get_cpu(emu->cur_loom, cpuid);

	emu_cpu_remove_thread(emu->cur_thread->cpu, emu->cur_thread);
	emu_cpu_add_thread(newcpu, emu->cur_thread);

	emu->cur_thread->cpu = newcpu;

	//dbg("cpu %d now runs %d\n", cpuid, emu->cur_thread->tid);
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

	newcpu = emu_get_cpu(emu->cur_loom, cpuid);

	/* It must not be running in any of the cpus */
	assert(emu_cpu_find_thread(thread->cpu, thread) == -1);
	assert(emu_cpu_find_thread(newcpu, thread) == -1);

	thread->cpu = newcpu;

	//dbg("thread %d switches to cpu %d by remote petition\n", tid,
	//		cpuid);
}

static void
ev_affinity(struct ovni_emu *emu)
{
	//emu_emit(emu);
	switch(emu->cur_ev->header.value)
	{
		case 's': ev_affinity_set(emu); break;
		case 'r': ev_affinity_remote(emu); break;
		default:
			dbg("unknown affinity event value %c\n",
					emu->cur_ev->header.value);
			break;
	}
}

void
hook_pre_ovni(struct ovni_emu *emu)
{
	//emu_emit(emu);

	switch(emu->cur_ev->header.class)
	{
		case 'H': ev_thread(emu); break;
		case 'A': ev_affinity(emu); break;
		case 'B': dbg("burst %c\n", emu->cur_ev->header.value); break;
		default:
			dbg("unknown ovni event class %c\n",
					emu->cur_ev->header.class);
			break;
	}

	//print_threads_state(emu);
}

static void
emit_thread_state(struct ovni_emu *emu)
{
	int row, st, tid;
	
	st = emu->cur_thread->state;
	row = emu->cur_thread->gindex + 1;
	tid = emu->cur_thread->tid;

	prv_ev_row(emu, row, PTT_THREAD_STATE, st);

	if(st == TH_ST_RUNNING)
		prv_ev_row(emu, row, PTT_THREAD_TID, tid);
	else
		prv_ev_row(emu, row, PTT_THREAD_TID, 0);
}

static void
emit_thread_count(struct ovni_emu *emu)
{
	int i, n, row, pid, tid;
	struct ovni_loom *loom;

	loom = emu->cur_loom;

	/* TODO: Use a table to quickly access the updated elements */

	/* Check every CPU looking for a change in nthreads */
	for(i=0; i<loom->ncpus; i++)
	{
		if(loom->cpu[i].last_nthreads != loom->cpu[i].nthreads)
		{
			/* Start at 1 */
			row = i + 1;
			n = loom->cpu[i].nthreads;
			prv_ev_row(emu, row, PTC_NTHREADS, n);

			pid = n == 1 ? loom->cpu[i].thread[0]->proc->pid : 1;
			prv_ev_row(emu, row, PTC_PROC_PID, pid);

			tid = n == 1 ? loom->cpu[i].thread[0]->tid : 1;
			prv_ev_row(emu, row, PTC_THREAD_TID, tid);
		}
	}

	/* Same with the virtual CPU */
	if(loom->vcpu.last_nthreads != loom->vcpu.nthreads)
	{
		/* Place the virtual CPU after the physical CPUs */
		row = loom->ncpus + 1;
		n = loom->vcpu.nthreads;
		prv_ev_row(emu, row, PTC_NTHREADS, n);

		pid = n == 1 ? loom->vcpu.thread[0]->proc->pid : 1;
		prv_ev_row(emu, row, PTC_PROC_PID, pid);

		tid = n == 1 ? loom->vcpu.thread[0]->tid : 1;
		prv_ev_row(emu, row, PTC_THREAD_TID, tid);
	}
}

static void
emit_current_pid(struct ovni_emu *emu)
{
	if(emu->cur_thread->cpu == NULL)
		return;

	prv_ev_autocpu(emu, PTC_PROC_PID, emu->cur_proc->pid);
}

void
hook_emit_ovni(struct ovni_emu *emu)
{
	switch(emu->cur_ev->header.class)
	{
		case 'H':
			emit_thread_state(emu);
			/* falltrough */
		case 'A':
			emit_thread_count(emu);
			//emit_current_pid(emu);
			break;
		default:
			break;
	}
}

void
hook_post_ovni(struct ovni_emu *emu)
{
	int i;
	struct ovni_loom *loom;

	loom = emu->cur_loom;

	/* Update last_nthreads in the CPUs */

	for(i=0; i<loom->ncpus; i++)
		loom->cpu[i].last_nthreads = loom->cpu[i].nthreads;

	loom->vcpu.last_nthreads = loom->vcpu.nthreads;
}
