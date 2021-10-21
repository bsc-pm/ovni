#include "ovni.h"
#include "emu.h"
#include "prv.h"
#include "chan.h"

#include <assert.h>

/* The emulator ovni module provides the execution model by tracking the thread
 * state and which threads run in each CPU */

/* --------------------------- init ------------------------------- */

void
hook_init_ovni(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	struct ovni_cpu *cpu;
	struct ovni_trace *trace;
	int i, row, type;
	FILE *prv_th, *prv_cpu;
	int64_t *clock;

	clock = &emu->delta_time;
	prv_th = emu->prv_thread;
	prv_cpu = emu->prv_cpu;
	trace = &emu->trace;

	/* Init the ovni channels in all threads */
	for(i=0; i<emu->total_nthreads; i++)
	{
		th = emu->global_thread[i];
		row = th->gindex + 1;

		chan_th_init(th, CHAN_OVNI_TID, CHAN_TRACK_NONE, row, prv_th, clock);
		chan_th_init(th, CHAN_OVNI_PID, CHAN_TRACK_NONE, row, prv_th, clock);
		chan_th_init(th, CHAN_OVNI_CPU, CHAN_TRACK_NONE, row, prv_th, clock);
		chan_th_init(th, CHAN_OVNI_STATE, CHAN_TRACK_NONE, row, prv_th, clock);

		chan_enable(&th->chan[CHAN_OVNI_TID], 1);
		chan_set(&th->chan[CHAN_OVNI_TID], th->tid);
		chan_enable(&th->chan[CHAN_OVNI_TID], 0);

		chan_enable(&th->chan[CHAN_OVNI_PID], 1);
		chan_set(&th->chan[CHAN_OVNI_PID], th->proc->pid);
		chan_enable(&th->chan[CHAN_OVNI_PID], 0);
	}

	/* Init the ovni channels in all cpus */
	for(i=0; i<emu->total_ncpus; i++)
	{
		cpu = emu->global_cpu[i];
		row = cpu->gindex + 1;

		chan_cpu_init(cpu, CHAN_OVNI_TID, CHAN_TRACK_NONE, row, prv_cpu, clock);
		chan_cpu_init(cpu, CHAN_OVNI_PID, CHAN_TRACK_NONE, row, prv_cpu, clock);
		chan_cpu_init(cpu, CHAN_OVNI_NTHREADS, CHAN_TRACK_NONE, row, prv_cpu, clock);

		chan_enable(&cpu->chan[CHAN_OVNI_TID], 1);
		chan_set(&cpu->chan[CHAN_OVNI_TID], 0);
		chan_emit(&cpu->chan[CHAN_OVNI_TID]);

		chan_enable(&cpu->chan[CHAN_OVNI_PID], 1);
		chan_set(&cpu->chan[CHAN_OVNI_PID], 0);
		chan_emit(&cpu->chan[CHAN_OVNI_PID]);

		chan_enable(&cpu->chan[CHAN_OVNI_NTHREADS], 1);
		chan_set(&cpu->chan[CHAN_OVNI_NTHREADS], cpu->nthreads);
		chan_emit(&cpu->chan[CHAN_OVNI_NTHREADS]);
	}
}

/* --------------------------- pre ------------------------------- */

static void
thread_set_channel_enabled(struct ovni_ethread *th, int enabled)
{
	struct ovni_chan *chan;
	int i;

	for(i=0; i<CHAN_MAX; i++)
	{
		chan = &th->chan[i];

		if(chan->track == CHAN_TRACK_TH_RUNNING)
		{
			chan_enable(chan, enabled);
			dbg("thread %d: %s chan %d\n",
					th->tid,
					enabled ? "enable" : "disable",
					i);
		}
	}
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
	int i, tid, pid, enabled;
	struct ovni_loom *loom;
	struct ovni_ethread *th;
	struct ovni_chan *chan;

	loom = emu->cur_loom;

	chan_set(&cpu->chan[CHAN_OVNI_NTHREADS], cpu->nthreads);

	th = NULL;

	if(cpu->nthreads == 0)
	{
		/* No thread running */
		tid = pid = 0;
	}
	else if(cpu->nthreads == 1)
	{
		/* Take the info from the unique thread */
		tid = cpu->thread[0]->tid;
		pid = cpu->thread[0]->proc->pid;
		th = cpu->thread[0];
	}
	else
	{
		/* Multiple threads */
		tid = pid = 1;
	}

	chan_set(&cpu->chan[CHAN_OVNI_TID], tid);
	chan_set(&cpu->chan[CHAN_OVNI_PID], pid);

//	enabled = (th != NULL && th->state == TH_ST_RUNNING ? 1 : 0);
//
//	/* Update all channels that need to follow the running thread */
//	for(i=0; i<CHAN_MAX; i++)
//	{
//		chan = &cpu->chan[i];
//
//		if(chan->track == CHAN_TRACK_TH_RUNNING)
//		{
//			chan_enable(chan, enabled);
//			dbg("cpu thread %d: %s chan %d\n",
//					tid,
//					enabled ? "enable" : "disable",
//					i);
//		}
//	}
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
		cpu->thread[i] = cpu->thread[j+1];

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

	/* Enable the cpu channel of the thread */
	chan_enable(&th->chan[CHAN_OVNI_CPU], 1);
	chan_set(&th->chan[CHAN_OVNI_CPU], cpu->gindex + 1);

	/* Enable thread TID and PID */
	chan_enable(&th->chan[CHAN_OVNI_TID], 1);
	chan_enable(&th->chan[CHAN_OVNI_PID], 1);

	/* Enable and set the thread state */
	chan_enable(&th->chan[CHAN_OVNI_STATE], 1);
	chan_set(&th->chan[CHAN_OVNI_STATE], th->state);
}

static void
pre_thread_end(struct ovni_emu *emu)
{
	struct ovni_ethread *th;

	th = emu->cur_thread;
	assert(th->state == TH_ST_RUNNING);
	assert(th->cpu);

	emu_cpu_remove_thread(emu, th->cpu, th);

	thread_set_state(th, TH_ST_DEAD);
	th->cpu = NULL;

	/* Disable the cpu channel of the thread */
	chan_enable(&th->chan[CHAN_OVNI_CPU], 0);

	/* Disable thread TID and PID */
	chan_enable(&th->chan[CHAN_OVNI_TID], 0);
	chan_enable(&th->chan[CHAN_OVNI_PID], 0);

	/* Disable thread state */
	chan_enable(&th->chan[CHAN_OVNI_STATE], 0);
}

static void
pre_thread_pause(struct ovni_emu *emu)
{
	assert(emu->cur_thread->state == TH_ST_RUNNING);
	assert(emu->cur_thread->cpu);

	emu_cpu_remove_thread(emu, emu->cur_thread->cpu, emu->cur_thread);

	thread_set_state(emu->cur_thread, TH_ST_PAUSED);
	chan_set(&emu->cur_thread->chan[CHAN_OVNI_STATE], emu->cur_thread->state);
}

static void
pre_thread_resume(struct ovni_emu *emu)
{
	assert(emu->cur_thread->state == TH_ST_PAUSED);
	assert(emu->cur_thread->cpu);

	emu_cpu_add_thread(emu, emu->cur_thread->cpu, emu->cur_thread);

	thread_set_state(emu->cur_thread, TH_ST_RUNNING);
	chan_set(&emu->cur_thread->chan[CHAN_OVNI_STATE], emu->cur_thread->state);
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

	if(emu->cur_thread->cpu == newcpu)
	{
		err("warning: thread %d affinity already set to cpu %d\n",
				emu->cur_thread->tid,
				emu->cur_thread->cpu->gindex);
		return;
	}

	emu_cpu_remove_thread(emu, emu->cur_thread->cpu, emu->cur_thread);
	emu_cpu_add_thread(emu, newcpu, emu->cur_thread);

	emu->cur_thread->cpu = newcpu;

	chan_set(&emu->cur_thread->chan[CHAN_OVNI_CPU],
			newcpu->gindex + 1);

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

	chan_set(&thread->chan[CHAN_OVNI_CPU],
			newcpu->gindex + 1);

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

/* --------------------------- post ------------------------------- */

void
hook_post_ovni(struct ovni_emu *emu)
{
}
