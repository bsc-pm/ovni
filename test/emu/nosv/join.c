/* Copyright (c) 2026 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"

/* Test the nosv_join(), nosv_wait(), nosv_join_all(), nosv_wait_all() API
 * events, introduced in the nOS-V model 2.7.0 */

static void
nosv_submit(void)
{
	instr_nosv_submit_enter();
	sleep_us(100);
	instr_nosv_submit_exit();
}

static void
nosv_pause(void)
{
	instr_nosv_task_pause(1, 0);
	sleep_us(100);
	instr_nosv_task_resume(1, 0);
}

static void
nosv_destroy(void)
{
	instr_nosv_destroy_enter();
	sleep_us(100);
	instr_nosv_destroy_exit();
}

/* Timeout: -1 Pause, 0 Check, X wait */
static void
wait(int timeout)
{
	instr_nosv_wait_enter();

	if (timeout == -1) {
		sleep_us(100);
		nosv_pause();
		sleep_us(100);
	} else if (timeout == 0) {
		/* Check */
	} else {
		sleep_us(100);
		nosv_submit();
		sleep_us(100);
		nosv_pause();
		sleep_us(100);
	}

	instr_nosv_wait_exit();
}

/* Timeout: -1 Pause, 0 Check, X wait */
static void
join(int timeout, int destroy)
{
	instr_nosv_join_enter();

	if (timeout == -1) {
		sleep_us(100);
		nosv_pause();
		sleep_us(100);
	} else if (timeout == 0) {
		// Check
	} else {
		sleep_us(100);
		nosv_submit();
		sleep_us(100);
		nosv_pause();
		sleep_us(100);
	}

	destroy = destroy || (timeout == -1);

	if (destroy)
		nosv_destroy();

	instr_nosv_join_exit();
}

/* Timeout: -1 Pause, 0 Check, X wait */
static void
wait_all(int timeout, int n)
{
	instr_nosv_wait_all_enter();

	if (timeout == -1) {
		sleep_us(100);
		nosv_pause();
		sleep_us(100);
	} else if (timeout == 0) {
		/* Check */
	} else {
		for (int i = 0; i < n; i++){
			sleep_us(100);
			nosv_submit();
			sleep_us(100);
			nosv_pause();
		}
		sleep_us(100);
	}

	instr_nosv_wait_all_exit();
}

/* Timeout: -1 Pause, 0 Check, X wait */
static void
join_all(int timeout, int destroy, int n)
{
	instr_nosv_wait_all_enter();

	sleep_us(100);

	if (timeout == -1) {
		nosv_pause();
		sleep_us(100);
	} else if (timeout == 0) {
		/* Check */
	} else {
		for (int i = 0; i < n; i++){
			nosv_submit();
			sleep_us(100);
			nosv_pause();
			sleep_us(100);
		}
	}

	if (destroy) {
		for (int i = 0; i < n; i++){
			nosv_destroy();
			sleep_us(100);
		}
	}

	instr_nosv_wait_all_exit();
}

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	instr_nosv_type_create(1);
	instr_nosv_task_create(1, 1);
	instr_nosv_task_execute(1, 0);

	/* timeout */
	wait(1);

	/* timeout + destroy */
	join(1, 1);

	/* timeout + 2 tasks */
	wait_all(1, 2);

	/* timeout + destroy + 2 tasks */
	join_all(1, 1, 2);

	instr_nosv_task_end(1, 0);
	instr_end();

	return 0;
}
