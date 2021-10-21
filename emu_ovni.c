#include "ovni.h"
#include "emu.h"
#include "prv.h"
#include "chan.h"

#include <assert.h>

/* The emulator ovni module provides the execution model by tracking the thread
 * state and which threads run in each CPU */

/* --------------------------- pre ------------------------------- */

static void
thread_set_channel_enabled(struct ovni_ethread *th, int enabled)
{
	int i;
	for(i=0; i<CHAN_MAX; i++)
		chan_enable(&th->chan[i], enabled);
}

static void
thread_set_state(struct ovni_ethread *th, int state)
{
	int enabled;

	th->state = state;

	enabled = (state == TH_ST_RUNNING ? 1 : 0);

	thread_set_channel_enabled(th, enabled);
}

void
update_cpu(struct ovni_emu *emu, struct ovni_cpu *cpu)
{
	struct ovni_loom *loom;

	loom = emu->cur_loom;

	if(loom->nupdated_cpus >= OVNI_MAX_CPU)
		abort();

	if(cpu->updated)
		return;

	cpu->updated = 1;
	loom->updated_cpu[loom->nupdated_cpus++] = cpu;
}

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
emu_cpu_add_thread(struct ovni_emu *emu, struct ovni_cpu *cpu, struct ovni_ethread *thread)
{
	/* Found, abort */
	if(emu_cpu_find_thread(cpu, thread) >= 0)
		abort();

	assert(cpu->nthreads < OVNI_MAX_THR);

	cpu->thread[cpu->nthreads++] = thread;

	update_cpu(emu, cpu);
}

void
emu_cpu_remove_thread(struct ovni_emu *emu, struct ovni_cpu *cpu, struct ovni_ethread *thread)
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

	update_cpu(emu, cpu);
}

static void
print_threads_state(struct ovni_loom *loom)
{
	struct ovni_cpu *cpu;
	int i, j;

	for(i=0; i<loom->ncpus; i++)
	{
		cpu = &loom->cpu[i];

		dbg("-- cpu %d runs %lu threads:", i, cpu->nthreads);
		for(j=0; j<cpu->nthreads; j++)
		{
			dbg(" %d", cpu->thread[j]->tid);
		}
		dbg("\n");
	}

	dbg("-- vcpu runs %lu threads:", loom->vcpu.nthreads);
	for(j=0; j<loom->vcpu.nthreads; j++)
	{
		dbg(" %d", loom->vcpu.thread[j]->tid);
	}
	dbg("\n");
}

static void
pre_thread_execute(struct ovni_emu *emu)
{
	struct ovni_cpu *cpu;
	struct ovni_ethread *th;
	int cpuid;

	th = emu->cur_thread;

	/* The thread cannot be already running */
	assert(th->state != TH_ST_RUNNING);

	cpuid = emu->cur_ev->payload.i32[0];
	//dbg("thread %d runs in cpuid %d\n", th->tid,
	//		cpuid);
	cpu = emu_get_cpu(emu->cur_loom, cpuid);

	thread_set_state(th, TH_ST_RUNNING);
	th->cpu = cpu;

	emu_cpu_add_thread(emu, cpu, th);
}

static void
pre_thread_end(struct ovni_emu *emu)
{
	assert(emu->cur_thread->state == TH_ST_RUNNING);
	assert(emu->cur_thread->cpu);

	emu_cpu_remove_thread(emu, emu->cur_thread->cpu, emu->cur_thread);

	thread_set_state(emu->cur_thread, TH_ST_DEAD);
	emu->cur_thread->cpu = NULL;
}

static void
pre_thread_pause(struct ovni_emu *emu)
{
	assert(emu->cur_thread->state == TH_ST_RUNNING);
	assert(emu->cur_thread->cpu);

	emu_cpu_remove_thread(emu, emu->cur_thread->cpu, emu->cur_thread);

	thread_set_state(emu->cur_thread, TH_ST_PAUSED);
}

static void
pre_thread_resume(struct ovni_emu *emu)
{
	assert(emu->cur_thread->state == TH_ST_PAUSED);
	assert(emu->cur_thread->cpu);

	emu_cpu_add_thread(emu, emu->cur_thread->cpu, emu->cur_thread);

	thread_set_state(emu->cur_thread, TH_ST_RUNNING);
}

static void
pre_thread(struct ovni_emu *emu)
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
		case 'x': pre_thread_execute(emu); break;
		case 'e': pre_thread_end(emu); break;
		case 'p': pre_thread_pause(emu); break;
		case 'r': pre_thread_resume(emu); break;
		default:
			  err("unknown thread event value %c\n",
					  ev->header.value);
			  exit(EXIT_FAILURE);
	}
}

static void
pre_affinity_set(struct ovni_emu *emu)
{
	int cpuid;
	struct ovni_cpu *newcpu;

	cpuid = emu->cur_ev->payload.i32[0];

	assert(emu->cur_thread->state == TH_ST_RUNNING);
	assert(emu->cur_thread->cpu);

	/* Migrate current cpu to the one at cpuid */
	newcpu = emu_get_cpu(emu->cur_loom, cpuid);

	emu_cpu_remove_thread(emu, emu->cur_thread->cpu, emu->cur_thread);
	emu_cpu_add_thread(emu, newcpu, emu->cur_thread);

	emu->cur_thread->cpu = newcpu;

	//dbg("cpu %d now runs %d\n", cpuid, emu->cur_thread->tid);
}

static void
pre_affinity_remote(struct ovni_emu *emu)
{
	int cpuid, tid;
	struct ovni_cpu *newcpu;
	struct ovni_ethread *thread;

	cpuid = emu->cur_ev->payload.i32[0];
	tid = emu->cur_ev->payload.i32[1];

	thread = emu_get_thread(emu->cur_proc, tid);

	assert(thread);

	/* The thread may still be running */
	assert(thread->state == TH_ST_RUNNING ||
			thread->state == TH_ST_PAUSED);

	/* It must have an assigned CPU */
	assert(thread->cpu);

	/* Migrate current cpu to the one at cpuid */
	newcpu = emu_get_cpu(emu->cur_loom, cpuid);

	/* If running, update the CPU thread lists */
	if(thread->state == TH_ST_RUNNING)
	{
		emu_cpu_remove_thread(emu, thread->cpu, thread);
		emu_cpu_add_thread(emu, newcpu, thread);
	}
	else
	{
		/* Otherwise, ensure that it is not in any CPU list */
		assert(emu_cpu_find_thread(thread->cpu, thread) == -1);
		assert(emu_cpu_find_thread(newcpu, thread) == -1);
	}

	/* Always set the assigned CPU in the thread */
	thread->cpu = newcpu;

	//dbg("thread %d switches to cpu %d by remote petition\n", tid,
	//		cpuid);
}

static void
pre_affinity(struct ovni_emu *emu)
{
	//emu_emit(emu);
	switch(emu->cur_ev->header.value)
	{
		case 's': pre_affinity_set(emu); break;
		case 'r': pre_affinity_remote(emu); break;
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

	if(emu->cur_ev->header.model != 'O')
		return;

	switch(emu->cur_ev->header.class)
	{
		case 'H': pre_thread(emu); break;
		case 'A': pre_affinity(emu); break;
		case 'B':
			  //dbg("burst %c\n", emu->cur_ev->header.value);
			  break;
		default:
			dbg("unknown ovni event class %c\n",
					emu->cur_ev->header.class);
			break;
	}

	//print_threads_state(emu);
}

/* --------------------------- emit ------------------------------- */

static void
emit_thread_state(struct ovni_emu *emu)
{
	int i, row;
	struct ovni_ethread *th;

	th = emu->cur_thread;
	row = th->gindex + 1;

	prv_ev_thread(emu, row, PTT_THREAD_STATE, th->state);

	if(th->state == TH_ST_RUNNING)
		prv_ev_thread(emu, row, PTT_THREAD_TID, th->tid);
	else
		prv_ev_thread(emu, row, PTT_THREAD_TID, 0);

	chan_set(&th->chan[CHAN_OVNI_TID], th->tid);

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
			row = loom->offset_ncpus + i + 1;
			n = loom->cpu[i].nthreads;
			prv_ev_cpu(emu, row, PTC_NTHREADS, n);

			pid = n == 1 ? loom->cpu[i].thread[0]->proc->pid : 1;
			prv_ev_cpu(emu, row, PTC_PROC_PID, pid);

			tid = n == 1 ? loom->cpu[i].thread[0]->tid : 1;
			prv_ev_cpu(emu, row, PTC_THREAD_TID, tid);
		}
	}

	/* Same with the virtual CPU */
	if(loom->vcpu.last_nthreads != loom->vcpu.nthreads)
	{
		/* Place the virtual CPU after the physical CPUs */
		row = loom->ncpus + 1;
		n = loom->vcpu.nthreads;
		prv_ev_cpu(emu, row, PTC_NTHREADS, n);

		pid = n == 1 ? loom->vcpu.thread[0]->proc->pid : 1;
		prv_ev_cpu(emu, row, PTC_PROC_PID, pid);

		tid = n == 1 ? loom->vcpu.thread[0]->tid : 1;
		prv_ev_cpu(emu, row, PTC_THREAD_TID, tid);
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
	int i;

	if(emu->cur_ev->header.model != 'O')
		return;

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

	/* Emit all enabled channels */
	for(i=0; i<CHAN_MAX; i++)
		chan_emit(&emu->cur_thread->chan[i]);
}

/* --------------------------- post ------------------------------- */

/* Reset thread state */
static void
post_virtual_thread(struct ovni_emu *emu)
{
	int i;
	struct ovni_loom *loom;

	loom = emu->cur_loom;

	/* Should be executed *before* we reset the CPUs updated flags */
	assert(loom->nupdated_cpus > 0);

	/* Update last_nthreads in the CPUs */
	for(i=0; i<loom->ncpus; i++)
		loom->cpu[i].last_nthreads = loom->cpu[i].nthreads;

	/* Fix the virtual CPU as well */
	loom->vcpu.last_nthreads = loom->vcpu.nthreads;
}

/* Reset CPU state */
static void
post_virtual_cpu(struct ovni_emu *emu)
{
	int i;
	struct ovni_loom *loom;

	loom = emu->cur_loom;

	for(i=0; i<loom->nupdated_cpus; i++)
	{
		/* Remove the update flags */
		assert(loom->updated_cpu[i]->updated == 1);
		loom->updated_cpu[i]->updated = 0;
	}

	/* Fix the virtual CPU as well */
	loom->vcpu.last_nthreads = loom->vcpu.nthreads;

	/* Restore 0 updated CPUs */
	loom->nupdated_cpus = 0;
}

static void
post_virtual(struct ovni_emu *emu)
{
	switch(emu->cur_ev->header.class)
	{
		case 'H':
			post_virtual_thread(emu);
			break;
		case 'C':
			post_virtual_cpu(emu);
			break;
		default:
			break;
	}
}

void
hook_post_ovni(struct ovni_emu *emu)
{
	switch(emu->cur_ev->header.model)
	{
		case '*':
			post_virtual(emu);
			break;
		default:
			break;
	}
}
