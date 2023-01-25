/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ovni_model.h"
#include "emu.h"



/* Sets the state of the thread and updates the thread tracking channels */
static void
thread_set_state(struct emu_thread *th, enum ethread_state state)
{
	/* The state must be updated when a cpu is set */
	if (th->cpu == NULL)
		die("thread_set_state: thread %d doesn't have a CPU\n",
				th->tid);

	dbg("thread_set_state: setting thread %d state %d\n",
			th->tid, state);

	th->state = state;

	th->is_running = (state == TH_ST_RUNNING) ? 1 : 0;

	th->is_active = (state == TH_ST_RUNNING
					|| state == TH_ST_COOLING
					|| state == TH_ST_WARMING)
					? 1
					: 0;

	chan_set(&th->chan[CHAN_OVNI_STATE], th->state);

	/* Enable or disable the thread channels that track the thread state */
	for (int i = 0; i < CHAN_MAX; i++)
		chan_tracking_update(&th->chan[i], th);

	dbg("thread_set_state: done\n");
}

static void
cpu_update_th_stats(struct emu_cpu *cpu)
{
	struct emu_thread *th = NULL;
	struct emu_thread *th_running = NULL;
	struct emu_thread *th_active = NULL;
	int active = 0, running = 0;

	DL_FOREACH(cpu->thread, th)
	{
		if (th->state == TH_ST_RUNNING) {
			th_running = th;
			running++;
			th_active = th;
			active++;
		} else if (th->state == TH_ST_COOLING || th->state == TH_ST_WARMING) {
			th_active = th;
			active++;
		}
	}

	cpu->nrunning_threads = running;
	cpu->nactive_threads = active;
	cpu->th_running = th_running;
	cpu->th_active = th_active;
}

static void
update_cpu(struct emu_cpu *cpu)
{
	dbg("updating cpu %s\n", cpu->name);

	/* Update the running and active threads first */
	cpu_update_th_stats(cpu);

	/* From the CPU channels we only need to manually update the number of
	 * threads running in the CPU */
	if (chan_get_st(&cpu->chan[CHAN_OVNI_NRTHREADS]) != (int) cpu->nrunning_threads)
		chan_set(&cpu->chan[CHAN_OVNI_NRTHREADS], (int) cpu->nrunning_threads);

	/* Update all tracking channels */
	for (int i = 0; i < CHAN_MAX; i++)
		emu_cpu_update_chan(cpu, &cpu->chan[i]);

	dbg("updating cpu %s complete!\n", cpu->name);
}

struct emu_cpu *
emu_get_cpu(struct emu_loom *loom, int cpuid)
{
	if (cpuid >= (int) loom->ncpus)
		die("emu_get_cpu: CPU index out of bounds\n");

	if (cpuid < 0)
		return &loom->vcpu;

	return &loom->cpu[cpuid];
}

static struct emu_thread *
emu_cpu_find_thread(struct emu_cpu *cpu, struct emu_thread *thread)
{
	struct emu_thread *p = NULL;

	DL_FOREACH(cpu->thread, p)
	{
		if (p == thread)
			return p;
	}

	return NULL;
}

/* Add the given thread to the list of threads assigned to the CPU */
static void
cpu_add_thread(struct emu_cpu *cpu, struct emu_thread *thread)
{
	/* Found, abort */
	if (emu_cpu_find_thread(cpu, thread) != NULL) {
		err("The thread %d is already assigned to %s\n",
				thread->tid, cpu->name);
		abort();
	}

	DL_APPEND(cpu->thread, thread);
	cpu->nthreads++;

	update_cpu(cpu);
}

static void
cpu_remove_thread(struct emu_cpu *cpu, struct emu_thread *thread)
{
	struct emu_thread *p = emu_cpu_find_thread(cpu, thread);

	/* Not found, abort */
	if (p == NULL) {
		err("cannot remove missing thread %d from cpu %s\n",
				thread->tid, cpu->name);
		abort();
	}

	DL_DELETE(cpu->thread, thread);
	cpu->nthreads--;

	update_cpu(cpu);
}

static void
cpu_migrate_thread(struct emu_cpu *cpu,
		struct emu_thread *thread,
		struct emu_cpu *newcpu)
{
	cpu_remove_thread(cpu, thread);
	cpu_add_thread(newcpu, thread);
}

static int
pre_thread_execute(struct emu *emu, struct emu_thread *th)
{
	/* The thread cannot be already running */
	if (th->state == TH_ST_RUNNING) {
		err("pre_thread_execute: thread %d already running\n", th->tid);
		return -1;
	}

	int cpuid = emu->ev->payload.i32[0];
	struct emu_cpu *cpu = emu_system_get_cpu(emu->loom, cpuid);
	dbg("pre_thread_execute: thread %d runs in CPU %s\n", th->tid, cpu->name);

	/* First set the CPU in the thread */
	thread_set_cpu(th, cpu);

	/* Then set the thread to running state */
	thread_set_state(th, TH_ST_RUNNING);

	/* And then add the thread to the CPU, so tracking channels see the
	 * updated thread state */
	cpu_add_thread(cpu, th);
}

//static void
//pre_thread_end(struct emu_thread *th)
//{
//	if (th->state != TH_ST_RUNNING && th->state != TH_ST_COOLING)
//		die("pre_thread_end: thread %d not running or cooling\n",
//				th->tid);
//
//	if (th->cpu == NULL)
//		die("pre_thread_end: thread %d doesn't have a CPU\n",
//				th->tid);
//
//	/* First update the thread state */
//	thread_set_state(th, TH_ST_DEAD);
//
//	/* Then remove it from the cpu, so channels are properly updated */
//	cpu_remove_thread(th->cpu, th);
//
//	thread_unset_cpu(th);
//}
//
//static void
//pre_thread_pause(struct emu_thread *th)
//{
//	if (th->state != TH_ST_RUNNING && th->state != TH_ST_COOLING)
//		die("pre_thread_pause: thread %d not running or cooling\n",
//				th->tid);
//
//	if (th->cpu == NULL)
//		die("pre_thread_pause: thread %d doesn't have a CPU\n",
//				th->tid);
//
//	thread_set_state(th, TH_ST_PAUSED);
//	update_cpu(th->cpu);
//}
//
//static void
//pre_thread_resume(struct emu_thread *th)
//{
//	if (th->state != TH_ST_PAUSED && th->state != TH_ST_WARMING)
//		die("pre_thread_resume: thread %d not paused or warming\n",
//				th->tid);
//
//	if (th->cpu == NULL)
//		die("pre_thread_resume: thread %d doesn't have a CPU\n",
//				th->tid);
//
//	thread_set_state(th, TH_ST_RUNNING);
//	update_cpu(th->cpu);
//}
//
//static void
//pre_thread_cool(struct emu_thread *th)
//{
//	if (th->state != TH_ST_RUNNING)
//		die("pre_thread_cool: thread %d not running\n",
//				th->tid);
//
//	if (th->cpu == NULL)
//		die("pre_thread_cool: thread %d doesn't have a CPU\n",
//				th->tid);
//
//	thread_set_state(th, TH_ST_COOLING);
//	update_cpu(th->cpu);
//}
//
//static void
//pre_thread_warm(struct emu_thread *th)
//{
//	if (th->state != TH_ST_PAUSED)
//		die("pre_thread_warm: thread %d not paused\n",
//				th->tid);
//
//	if (th->cpu == NULL)
//		die("pre_thread_warm: thread %d doesn't have a CPU\n",
//				th->tid);
//
//	thread_set_state(th, TH_ST_WARMING);
//	update_cpu(th->cpu);
//}

static int
pre_thread(struct emu *emu)
{
	struct emu_thread *th = emu->thread;
	struct emu_ev *ev = emu->ev;

	switch (ev->v) {
		case 'C': /* create */
			dbg("thread %d creates a new thread at cpu=%d with args=%x %x\n",
					th->tid,
					ev->payload->u32[0],
					ev->payload->u32[1],
					ev->payload->u32[2]);

			break;
		case 'x':
			return pre_thread_execute(emu, th);
//		case 'e':
//			return pre_thread_end(th);
//		case 'p':
//			return pre_thread_pause(th);
//		case 'r':
//			return pre_thread_resume(th);
//		case 'c':
//			return pre_thread_cool(th);
//		case 'w':
//			return pre_thread_warm(th);
		default:
			err("unknown thread event value %c\n", ev->v);
//			return -1;
	}
	return 0;
}

//static void
//pre_affinity_set(struct emu *emu)
//{
//	struct emu_thread *th = emu->cur_thread;
//	int cpuid = emu->ev->payload.i32[0];
//
//	if (th->cpu == NULL)
//		edie(emu, "pre_affinity_set: thread %d doesn't have a CPU\n",
//				th->tid);
//
//	if (!th->is_active)
//		edie(emu, "pre_affinity_set: thread %d is not active\n",
//				th->tid);
//
//	/* Migrate current cpu to the one at cpuid */
//	struct emu_cpu *newcpu = emu_get_cpu(emu->cur_loom, cpuid);
//
//	/* The CPU is already properly set, return */
//	if (th->cpu == newcpu)
//		return;
//
//	cpu_migrate_thread(th->cpu, th, newcpu);
//	thread_migrate_cpu(th, newcpu);
//
//	// dbg("cpu %d now runs %d\n", cpuid, th->tid);
//}
//
//static void
//pre_affinity_remote(struct emu *emu)
//{
//	int32_t cpuid = emu->ev->payload.i32[0];
//	int32_t tid = emu->ev->payload.i32[1];
//
//	struct emu_thread *remote_th = emu_get_thread(emu->cur_proc, tid);
//
//	if (remote_th == NULL) {
//		/* Search the thread in other processes of the loom if
//		 * not found in the current one */
//		struct emu_loom *loom = emu->cur_loom;
//
//		for (size_t i = 0; i < loom->nprocs; i++) {
//			struct ovni_eproc *proc = &loom->proc[i];
//
//			/* Skip the current process */
//			if (proc == emu->cur_proc)
//				continue;
//
//			remote_th = emu_get_thread(proc, tid);
//
//			if (remote_th)
//				break;
//		}
//
//		if (remote_th == NULL) {
//			err("thread tid %d not found: cannot set affinity remotely\n",
//					tid);
//			abort();
//		}
//	}
//
//	/* The remote_th cannot be in states dead or unknown */
//	if (remote_th->state == TH_ST_DEAD)
//		edie(emu, "pre_affinity_remote: remote thread %d in state DEAD\n",
//				remote_th->tid);
//
//	if (remote_th->state == TH_ST_UNKNOWN)
//		edie(emu, "pre_affinity_remote: remote thread %d in state UNKNOWN\n",
//				remote_th->tid);
//
//	/* It must have an assigned CPU */
//	if (remote_th->cpu == NULL)
//		edie(emu, "pre_affinity_remote: remote thread %d has no CPU\n",
//				remote_th->tid);
//
//	/* Migrate current cpu to the one at cpuid */
//	struct emu_cpu *newcpu = emu_get_cpu(emu->cur_loom, cpuid);
//
//	cpu_migrate_thread(remote_th->cpu, remote_th, newcpu);
//	thread_migrate_cpu(remote_th, newcpu);
//
//	// dbg("remote_th %d switches to cpu %d by remote petition\n", tid,
//	//		cpuid);
//}
//
//static void
//pre_affinity(struct emu *emu)
//{
//	// emu_emit(emu);
//	switch (emu->ev->v) {
//		case 's':
//			pre_affinity_set(emu);
//			break;
//		case 'r':
//			pre_affinity_remote(emu);
//			break;
//		default:
//			dbg("unknown affinity event value %c\n",
//					emu->ev->v);
//			break;
//	}
//}
//
//static int
//compare_int64(const void *a, const void *b)
//{
//	int64_t aa = *(const int64_t *) a;
//	int64_t bb = *(const int64_t *) b;
//
//	if (aa < bb)
//		return -1;
//	else if (aa > bb)
//		return +1;
//	else
//		return 0;
//}
//
//static void
//pre_burst(struct emu *emu)
//{
//	int64_t dt = 0;
//
//	UNUSED(dt);
//
//	struct emu_thread *th = emu->cur_thread;
//
//	if (th->nbursts >= MAX_BURSTS) {
//		err("too many bursts: ignored\n");
//		return;
//	}
//
//	th->burst_time[th->nbursts++] = emu->delta_time;
//	if (th->nbursts == MAX_BURSTS) {
//		int n = MAX_BURSTS - 1;
//		int64_t deltas[MAX_BURSTS - 1];
//		for (int i = 0; i < n; i++) {
//			deltas[i] = th->burst_time[i + 1] - th->burst_time[i];
//		}
//
//		qsort(deltas, n, sizeof(int64_t), compare_int64);
//
//		double avg = 0.0;
//		double maxdelta = 0;
//		for (int i = 0; i < n; i++) {
//			if (deltas[i] > maxdelta)
//				maxdelta = deltas[i];
//			avg += deltas[i];
//		}
//
//		avg /= (double) n;
//		double median = deltas[n / 2];
//
//		err("%s burst stats: median %.0f ns, avg %.1f ns, max %.0f ns\n",
//				emu->cur_loom->dname, median, avg, maxdelta);
//
//		th->nbursts = 0;
//	}
//}
//
//static void
//pre_flush(struct emu *emu)
//{
//	struct emu_thread *th = emu->cur_thread;
//	struct ovni_chan *chan_th = &th->chan[CHAN_OVNI_FLUSH];
//
//	switch (emu->ev->v) {
//		case '[':
//			chan_push(chan_th, ST_OVNI_FLUSHING);
//			break;
//		case ']':
//			chan_pop(chan_th, ST_OVNI_FLUSHING);
//			break;
//		default:
//			err("unexpected value '%c' (expecting '[' or ']')\n",
//					emu->ev->v);
//			abort();
//	}
//}

static int
hook_pre_ovni(struct emu *emu)
{
	if (emu->ev->m != 'O')
		return -1;

	switch (emu->ev->c) {
		case 'H':
			return pre_thread(emu);
//		case 'A':
//			pre_affinity(emu);
//			break;
//		case 'B':
//			pre_burst(emu);
//			break;
//		case 'F':
//			pre_flush(emu);
//			break;
		default:
			err("unknown ovni event category %c\n",
					emu->ev->c);
			return -1;
	}

	return 0;
}

int
ovni_model_event(void *ptr)
{
	struct emu *emu = emu_get(ptr);
	if (emu->ev->m != 'O') {
		err("unexpected event model %c\n", emu->ev->m);
		return -1;
	}

	err("got ovni event '%s'\n", emu->ev->mcv);
	if (hook_pre_ovni(emu) != 0) {
		err("ovni_model_event: failed to process event\n");
		return -1;
	}

	return 0;
}
