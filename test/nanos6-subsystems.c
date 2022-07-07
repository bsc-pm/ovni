/*
 * Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
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

#define _GNU_SOURCE

#include "test/common.h"
#include "test/nanos6.h"

int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));
	instr_start(rank, nranks);

	int us = 500;

	int32_t typeid = 1;
	int32_t taskid = 1;

	type_create(typeid);
	task_begin(taskid, typeid);

	sched_receive_task();
	usleep(us);
	sched_assign_task();
	usleep(us);
	sched_self_assign_task();
	usleep(us);
	sched_hungry();
	usleep(us);
	sched_fill();
	usleep(us);
	sched_server_enter();
	usleep(us);
	sched_server_exit();
	usleep(us);
	sched_submit_enter();
	usleep(us);
	sched_submit_exit();
	usleep(us);
	enter_submit_task();
	usleep(us);
	exit_submit_task();
	usleep(us);
	block_enter(taskid);
	usleep(us);
	block_exit(taskid);
	usleep(us);
	waitfor_enter(taskid);
	usleep(us);
	waitfor_exit(taskid);
	usleep(us);
	register_accesses_enter();
	usleep(us);
	register_accesses_exit();
	usleep(us);
	unregister_accesses_enter();
	usleep(us);
	unregister_accesses_exit();
	usleep(us);
	task_wait_enter(taskid);
	usleep(us);
	task_wait_exit(taskid);
	usleep(us);
	spawn_function_enter();
	usleep(us);
	spawn_function_exit();
	usleep(us);

	instr_end();

	return 0;
}

