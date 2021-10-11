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

		/* All threads begin in unknown state */
		chan_enable(&th->chan[CHAN_OVNI_STATE], 1);
		chan_set(&th->chan[CHAN_OVNI_STATE], TH_ST_UNKNOWN);
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
thread_update_channels(struct ovni_ethread *th)
{
	struct ovni_chan *chan;
	int i, st, enabled, is_running, is_unpaused;

	st = th->state;
	is_running = (st == TH_ST_RUNNING);
	is_unpaused = (st == TH_ST_RUNNING
			|| st == TH_ST_COOLING
			|| st == TH_ST_WARMING);

	for(i=0; i<CHAN_MAX; i++)
	{
		chan = &th->chan[i];

		switch (chan->track)
		{
			case CHAN_TRACK_TH_RUNNING:
				enabled = is_running ? 1 : 0;
				break;
			case CHAN_TRACK_TH_UNPAUSED:
				enabled = is_unpaused ? 1 : 0;
				break;
			default:
				continue;
		}

		/* The channel is already in the proper state */
		if(chan_is_enabled(chan) == enabled)
			continue;

		dbg("thread %d changes state to %d: chan %d enabled=%d\n",
				th->tid, th->state, i, enabled);
		chan_enable(chan, enabled);
	}
}

static void
thread_set_state(struct ovni_ethread *th, int state)
{
	int enabled;

	th->state = state;

	chan_set(&th->chan[CHAN_OVNI_STATE], th->state);
	
	/* Enable or disable the thread channels that track the thread state */
	thread_update_channels(th);
}

void
update_cpu(struct ovni_emu *emu, struct ovni_cpu *cpu)
{
	int i, tid, pid, enabled, nrunning;
	struct ovni_loom *loom;
	struct ovni_ethread *th, *last_th;
	struct ovni_chan *chan;

	loom = emu->cur_loom;

	/* Count running threads */
	for(i=0, nrunning=0; i<cpu->nthreads; i++)
	{
		th = cpu->thread[i];
		if(th->state == TH_ST_RUNNING)
		{
			last_th = th;
			nrunning++;
		}
	}

	chan_set(&cpu->chan[CHAN_OVNI_NTHREADS], nrunning);

	th = NULL;

	if(nrunning == 0)
	{
		/* No thread running */
		tid = pid = 0;
	}
	else if(nrunning == 1)
	{
		/* Take the info from the running thread */
		tid = last_th->tid;
		pid = last_th->proc->pid;
		th = last_th;
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

static void
cpu_add_thread(struct ovni_emu *emu, struct ovni_cpu *cpu, struct ovni_ethread *thread)
{
	/* Found, abort */
	if(emu_cpu_find_thread(cpu, thread) >= 0)
		abort();

	assert(cpu->nthreads < OVNI_MAX_THR);

	cpu->thread[cpu->nthreads++] = thread;

	update_cpu(emu, cpu);
}

static void
cpu_remove_thread(struct ovni_emu *emu, struct ovni_cpu *cpu, struct ovni_ethread *thread)
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

void
cpu_migrate_thread(struct ovni_emu *emu,
		struct ovni_cpu *cpu,
		struct ovni_ethread *thread,
		struct ovni_cpu *newcpu)
{

	cpu_remove_thread(emu, cpu, thread);
	cpu_add_thread(emu, newcpu, thread);
}

/* Sets the thread assigned CPU to the given one.
 * Precondition: the thread CPU must be null */
void
thread_set_cpu(struct ovni_ethread *th, struct ovni_cpu *cpu)
{
	assert(th->cpu == NULL);
	th->cpu = cpu;

	chan_enable(&th->chan[CHAN_OVNI_CPU], 1);
	chan_set(&th->chan[CHAN_OVNI_CPU], cpu->gindex + 1);
}

/* Unsets the thread assigned CPU.
 * Precondition: the thread CPU must be not null */
void
thread_unset_cpu(struct ovni_ethread *th)
{
	assert(th->cpu != NULL);
	th->cpu = NULL;

	chan_enable(&th->chan[CHAN_OVNI_CPU], 0);
}

/* Migrates the thread assigned CPU to the given one.
 * Precondition: the thread CPU must be not null */
void
thread_migrate_cpu(struct ovni_ethread *th, struct ovni_cpu *cpu)
{
	assert(th->cpu != NULL);
	th->cpu = cpu;

	assert(chan_is_enabled(&th->chan[CHAN_OVNI_CPU]));
	chan_set(&th->chan[CHAN_OVNI_CPU], cpu->gindex + 1);
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
pre_thread_execute(struct ovni_emu *emu, struct ovni_ethread *th)
{
	struct ovni_cpu *cpu;
	int cpuid;

	/* The thread cannot be already running */
	assert(th->state != TH_ST_RUNNING);

	cpuid = emu->cur_ev->payload.i32[0];
	//dbg("thread %d runs in cpuid %d\n", th->tid,
	//		cpuid);
	cpu = emu_get_cpu(emu->cur_loom, cpuid);

	thread_set_state(th, TH_ST_RUNNING);
	thread_set_cpu(th, cpu);

	cpu_add_thread(emu, cpu, th);

	/* Enable thread TID and PID */
	chan_enable(&th->chan[CHAN_OVNI_TID], 1);
	chan_enable(&th->chan[CHAN_OVNI_PID], 1);
}

static void
pre_thread_end(struct ovni_emu *emu, struct ovni_ethread *th)
{
	assert(th->state == TH_ST_RUNNING);
	assert(th->cpu);

	cpu_remove_thread(emu, th->cpu, th);

	thread_set_state(th, TH_ST_DEAD);
	thread_unset_cpu(th);

	/* Disable thread TID and PID */
	chan_enable(&th->chan[CHAN_OVNI_TID], 0);
	chan_enable(&th->chan[CHAN_OVNI_PID], 0);
}

static void
pre_thread_pause(struct ovni_emu *emu, struct ovni_ethread *th)
{
	assert(th->state == TH_ST_RUNNING || th->state == TH_ST_COOLING);
	assert(th->cpu);

	thread_set_state(th, TH_ST_PAUSED);
}

static void
pre_thread_resume(struct ovni_emu *emu, struct ovni_ethread *th)
{
	assert(th->state == TH_ST_PAUSED || th->state == TH_ST_WARMING);
	assert(th->cpu);

	thread_set_state(th, TH_ST_RUNNING);
}

static void
pre_thread_cool(struct ovni_emu *emu, struct ovni_ethread *th)
{
	assert(th->state == TH_ST_RUNNING);
	assert(th->cpu);

	thread_set_state(th, TH_ST_COOLING);
}

static void
pre_thread_warm(struct ovni_emu *emu, struct ovni_ethread *th)
{
	assert(th->state == TH_ST_PAUSED);
	assert(th->cpu);

	thread_set_state(th, TH_ST_WARMING);
}

static void
pre_thread(struct ovni_emu *emu)
{
	struct ovni_ev *ev;
	struct ovni_cpu *cpu;
	struct ovni_ethread *th, *remote_thread;
	int i;

	//emu_emit(emu);

	th = emu->cur_thread;
	cpu = th->cpu;
	ev = emu->cur_ev;

	switch(ev->header.value)
	{
		case 'C': /* create */
			dbg("thread %d creates a new thread at cpu=%d with args=%x %x\n",
					th->tid,
					ev->payload.u32[0],
					ev->payload.u32[1],
					ev->payload.u32[2]);

			break;
		case 'x': pre_thread_execute(emu, th); break;
		case 'e': pre_thread_end(emu, th); break;
		case 'p': pre_thread_pause(emu, th); break;
		case 'r': pre_thread_resume(emu, th); break;
		case 'c': pre_thread_cool(emu, th); break;
		case 'w': pre_thread_warm(emu, th); break;
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
	struct ovni_ethread *th;

	th = emu->cur_thread;
	cpuid = emu->cur_ev->payload.i32[0];

	assert(th->cpu);
	assert(th->state == TH_ST_RUNNING
			|| th->state == TH_ST_COOLING
			|| th->state == TH_ST_WARMING);

	/* Migrate current cpu to the one at cpuid */
	newcpu = emu_get_cpu(emu->cur_loom, cpuid);

	if(th->cpu == newcpu)
	{
		err("warning: thread %d affinity already set to cpu %d\n",
				th->tid,
				th->cpu->gindex);
		return;
	}

	cpu_migrate_thread(emu, th->cpu, th, newcpu);
	thread_migrate_cpu(th, newcpu);

	//dbg("cpu %d now runs %d\n", cpuid, th->tid);
}

static void
pre_affinity_remote(struct ovni_emu *emu)
{
	int cpuid, tid;
	struct ovni_cpu *newcpu;
	struct ovni_ethread *remote_th;

	cpuid = emu->cur_ev->payload.i32[0];
	tid = emu->cur_ev->payload.i32[1];

	remote_th = emu_get_thread(emu->cur_proc, tid);

	assert(remote_th);

	/* The remote_th cannot be in states dead or unknown */
	assert(remote_th->state != TH_ST_DEAD
			&& remote_th->state != TH_ST_UNKNOWN);

	/* It must have an assigned CPU */
	assert(remote_th->cpu);

	/* Migrate current cpu to the one at cpuid */
	newcpu = emu_get_cpu(emu->cur_loom, cpuid);

	cpu_migrate_thread(emu, remote_th->cpu, remote_th, newcpu);
	thread_migrate_cpu(remote_th, newcpu);

	//dbg("remote_th %d switches to cpu %d by remote petition\n", tid,
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
pre_burst(struct ovni_emu *emu)
{
	struct ovni_ethread *th;
	int64_t dt;

	th = emu->cur_thread;

	if(th->nbursts >= MAX_BURSTS)
	{
		err("too many bursts: ignored\n");
		return;
	}

	th->burst_time[th->nbursts] = emu->delta_time;
	if(th->nbursts > 0)
	{
		dt = th->burst_time[th->nbursts] -
			th->burst_time[th->nbursts - 1];

		err("burst delta time %ld ns\n", dt);
	}

	th->nbursts++;
}

void
hook_pre_ovni(struct ovni_emu *emu)
{
	//emu_emit(emu);

	if(emu->cur_ev->header.model != 'O')
		return;

	switch(emu->cur_ev->header.category)
	{
		case 'H': pre_thread(emu); break;
		case 'A': pre_affinity(emu); break;
		case 'B': pre_burst(emu); break;
		default:
			dbg("unknown ovni event category %c\n",
					emu->cur_ev->header.category);
			break;
	}

	//print_threads_state(emu);
}

/* --------------------------- post ------------------------------- */

void
hook_post_ovni(struct ovni_emu *emu)
{
}
