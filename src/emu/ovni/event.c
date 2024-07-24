/* Copyright (c) 2021-2024 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "ovni_priv.h"
#include <stdint.h>
#include <stdlib.h>
#include "chan.h"
#include "common.h"
#include "cpu.h"
#include "emu.h"
#include "emu_ev.h"
#include "extend.h"
#include "loom.h"
#include "model_thread.h"
#include "ovni.h"
#include "proc.h"
#include "thread.h"
#include "value.h"
#include "mark.h"

static int
pre_thread_execute(struct emu *emu, struct thread *th)
{
	/* The thread cannot be already running */
	if (th->state == TH_ST_RUNNING) {
		err("cannot execute thread %d, is already running", th->tid);
		return -1;
	}

	if (emu->ev->payload_size < 4) {
		err("missing payload in thread %d execute event", th->tid);
		return -1;
	}

	int index = emu->ev->payload->i32[0];
	struct cpu *cpu = loom_get_cpu(emu->loom, index);

	if (cpu == NULL) {
		err("cannot find CPU with index %d in loom %s",
				index, emu->loom->id);
		return -1;
	}

	dbg("thread %d runs in %s", th->tid, cpu->name);

	/* First set the CPU in the thread */
	if (thread_set_cpu(th, cpu) != 0) {
		err("thread_set_cpu failed");
		return -1;
	}

	/* Then set the thread to running state */
	if (thread_set_state(th, TH_ST_RUNNING) != 0) {
		err("thread_set_state failed");
		return -1;
	}

	/* And then add the thread to the CPU, so tracking channels see the
	 * updated thread state */
	if (cpu_add_thread(cpu, th) != 0) {
		err("cpu_add_thread failed");
		return -1;
	}

	return 0;
}

static int
pre_thread_end(struct thread *th)
{
	if (th->state != TH_ST_RUNNING && th->state != TH_ST_COOLING) {
		err("cannot end thread %d: state not running or cooling",
				th->tid);
		return -1;
	}

	if (thread_set_state(th, TH_ST_DEAD) != 0) {
		err("cannot set thread %d state", th->tid);
		return -1;
	}

	if (cpu_remove_thread(th->cpu, th) != 0) {
		err("cannot remove thread %d from %s",
				th->tid, th->cpu->name);
		return -1;
	}

	if (thread_unset_cpu(th) != 0) {
		err("cannot unset cpu from thread %d", th->tid);
		return -1;
	}

	return 0;
}

static int
pre_thread_pause(struct thread *th)
{
	if (th->state != TH_ST_RUNNING && th->state != TH_ST_COOLING) {
		err("cannot pause thread %d: state not running or cooling",
				th->tid);
		return -1;
	}

	if (thread_set_state(th, TH_ST_PAUSED) != 0) {
		err("cannot set thread %d state", th->tid);
		return -1;
	}

	if (cpu_update(th->cpu) != 0) {
		err("cpu_update failed for %s", th->cpu->name);
		return -1;
	}

	return 0;
}

static int
pre_thread_resume(struct thread *th)
{
	if (th->state != TH_ST_PAUSED && th->state != TH_ST_WARMING) {
		err("cannot resume thread %d: state not paused or warming",
				th->tid);
		return -1;
	}

	if (thread_set_state(th, TH_ST_RUNNING) != 0) {
		err("cannot set thread %d state", th->tid);
		return -1;
	}

	if (cpu_update(th->cpu) != 0) {
		err("cpu_update failed for %s", th->cpu->name);
		return -1;
	}

	return 0;
}

static int
pre_thread_cool(struct thread *th)
{
	if (th->state != TH_ST_RUNNING) {
		err("cannot cool thread %d: state not running", th->tid);
		return -1;
	}

	if (thread_set_state(th, TH_ST_COOLING) != 0) {
		err("cannot set thread %d state", th->tid);
		return -1;
	}

	if (cpu_update(th->cpu) != 0) {
		err("cpu_update failed for %s", th->cpu->name);
		return -1;
	}

	return 0;
}

static int
pre_thread_warm(struct thread *th)
{
	if (th->state != TH_ST_PAUSED) {
		err("cannot warm thread %d: state not paused", th->tid);
		return -1;
	}

	if (thread_set_state(th, TH_ST_WARMING) != 0) {
		err("cannot set thread %d state", th->tid);
		return -1;
	}

	if (cpu_update(th->cpu) != 0) {
		err("cpu_update failed for %s", th->cpu->name);
		return -1;
	}

	return 0;
}

static int
pre_thread(struct emu *emu)
{
	struct thread *th = emu->thread;
	struct emu_ev *ev = emu->ev;

	switch (ev->v) {
		case 'C': /* create */
			dbg("thread %d creates a new thread at cpu=%d with args=%x %x",
					th->tid,
					ev->payload->u32[0],
					ev->payload->u32[1],
					ev->payload->u32[2]);

			break;
		case 'x':
			return pre_thread_execute(emu, th);
		case 'e':
			return pre_thread_end(th);
		case 'p':
			return pre_thread_pause(th);
		case 'r':
			return pre_thread_resume(th);
		case 'c':
			return pre_thread_cool(th);
		case 'w':
			return pre_thread_warm(th);
		default:
			err("unknown thread event value %c", ev->v);
			return -1;
	}
	return 0;
}

static int
pre_affinity_set(struct emu *emu)
{
	struct thread *th = emu->thread;

	if (th->cpu == NULL) {
		err("thread %d doesn't have CPU set", th->tid);
		return -1;
	}

	if (!th->is_active) {
		err("thread %d is not active", th->tid);
		return -1;
	}

	if (emu->ev->payload_size != 4) {
		err("unexpected payload size %zd", emu->ev->payload_size);
		return -1;
	}

	/* Migrate current cpu to the one at index */
	int index = emu->ev->payload->i32[0];
	struct cpu *newcpu = loom_get_cpu(emu->loom, index);

	if (newcpu == NULL) {
		err("cannot find cpu with index %d", index);
		return -1;
	}

	/* The CPU is already properly set, return */
	if (th->cpu == newcpu)
		return 0;

	if (cpu_migrate_thread(th->cpu, th, newcpu) != 0) {
		err("cpu_migrate_thread() failed");
		return -1;
	}

	if (thread_migrate_cpu(th, newcpu) != 0) {
		err("thread_migrate_cpu() failed");
		return -1;
	}

	dbg("thread %d now runs in %s", th->tid, newcpu->name);

	return 0;
}

static int
pre_affinity_remote(struct emu *emu)
{
	if (emu->ev->payload_size != 8) {
		err("unexpected payload size %zd", emu->ev->payload_size);
		return -1;
	}

	int32_t index = emu->ev->payload->i32[0];
	int32_t tid = emu->ev->payload->i32[1];

	struct thread *remote_th = proc_find_thread(emu->proc, tid);

	/* Search the thread in other processes of the loom if
	 * not found in the current one */
	if (remote_th == NULL)
		remote_th = loom_find_thread(emu->loom, tid);

	if (remote_th == NULL) {
		err("thread %d not found", tid);
		return -1;
	}

	/* The remote_th cannot be in states dead or unknown */
	if (remote_th->state == TH_ST_DEAD) {
		err("thread %d is dead", tid);
		return -1;
	}

	if (remote_th->state == TH_ST_UNKNOWN) {
		err("thread %d in state unknown", tid);
		return -1;
	}

	/* It movni have an assigned CPU */
	if (remote_th->cpu == NULL) {
		err("thread %d has no CPU", tid);
		return -1;
	}

	/* Migrate current cpu to the one at index */
	struct cpu *newcpu = loom_get_cpu(emu->loom, index);
	if (newcpu == NULL) {
		err("cannot find CPU with index %d", index);
		return -1;
	}

	if (cpu_migrate_thread(remote_th->cpu, remote_th, newcpu) != 0) {
		err("cpu_migrate_thread() failed");
		return -1;
	}

	if (thread_migrate_cpu(remote_th, newcpu) != 0) {
		err("thread_migrate_cpu() failed");
		return -1;
	}

	dbg("remote_th %d remotely switches to cpu %d", tid, index);

	return 0;
}

static int
pre_affinity(struct emu *emu)
{
	switch (emu->ev->v) {
		case 's':
			return pre_affinity_set(emu);
		case 'r':
			return pre_affinity_remote(emu);
		default:
			err("unknown affinity event value %c",
					emu->ev->v);
			return -1;
	}

	return 0;
}

static int
pre_cpu(struct emu *emu)
{
	switch (emu->ev->v) {
		case 'n':
			warn("ignoring old event OCn");
			return 0;
		default:
			err("unknown cpu event value %c",
					emu->ev->v);
			return -1;
	}

	return 0;
}

static int
compare_int64(const void *a, const void *b)
{
	int64_t aa = *(const int64_t *) a;
	int64_t bb = *(const int64_t *) b;

	if (aa < bb)
		return -1;
	else if (aa > bb)
		return +1;
	else
		return 0;
}

static int
pre_burst(struct emu *emu)
{
	struct ovni_thread *th = EXT(emu->thread, 'O');

	if (th->nbursts >= MAX_BURSTS) {
		err("too many bursts");
		return -1;
	}

	th->burst_time[th->nbursts++] = emu->ev->dclock;

	if (th->nbursts != MAX_BURSTS)
		return 0;

	int n = MAX_BURSTS - 1;
	int64_t deltas[MAX_BURSTS - 1];
	for (int i = 0; i < n; i++)
		deltas[i] = th->burst_time[i + 1] - th->burst_time[i];

	qsort(deltas, (size_t) n, sizeof(int64_t), compare_int64);

	double avg = 0.0;
	double maxdelta = 0;
	for (int i = 0; i < n; i++) {
		if ((double) deltas[i] > maxdelta)
			maxdelta = (double) deltas[i];
		avg += (double) deltas[i];
	}

	avg /= (double) n;
	double median = (double) deltas[n / 2];

	info("%s burst stats: median/avg/max = %3.0f/%3.0f/%3.0f ns",
			emu->loom->id, median, avg, maxdelta);

	if (median > 100)
		warn("large median burst time of %.0f ns, expect overhead", median);

	th->nbursts = 0;

	return 0;
}

static int
pre_flush(struct emu *emu)
{
	struct ovni_thread *th = EXT(emu->thread, 'O');
	struct chan *flush = &th->m.ch[CH_FLUSH];

	switch (emu->ev->v) {
		case '[':
			if (chan_set(flush, value_int64(1)) != 0) {
				err("chan_set failed");
				return -1;
			}
			th->flush_start = emu->ev->dclock;
			break;
		case ']':
			if (chan_set(flush, value_null()) != 0) {
				err("chan_set failed");
				return -1;
			}
			int64_t flush_ns = emu->ev->dclock - th->flush_start;
			double flush_ms = (double) flush_ns * 1e-6;
			/* Avoid last flush warnings */
			if (flush_ms > 10.0 && emu->thread->is_running)
				warn("large flush of %.1f ms at dclock=%"PRIi64" ns in tid=%d",
						flush_ms,
						emu->ev->dclock,
						emu->thread->tid);
			break;
		default:
			err("unexpected value '%c' (expecting '[' or ']')",
					emu->ev->v);
			return -1;
	}

	return 0;
}

int
model_ovni_event(struct emu *emu)
{
	if (emu->ev->m != 'O') {
		err("unexpected event model %c", emu->ev->m);
		return -1;
	}

	if (emu->thread->is_out_of_cpu) {
		err("current thread %d out of CPU", emu->thread->tid);
		return -1;
	}

	switch (emu->ev->c) {
		case 'H':
			return pre_thread(emu);
		case 'A':
			return pre_affinity(emu);
		case 'B':
			return pre_burst(emu);
		case 'C':
			return pre_cpu(emu);
		case 'F':
			return pre_flush(emu);
		case 'U':
			/* Ignore sorting events */
			return 0;
		case 'M':
			return mark_event(emu);
		default:
			err("unknown ovni event category %c",
					emu->ev->c);
			return -1;
	}

	return 0;
}
