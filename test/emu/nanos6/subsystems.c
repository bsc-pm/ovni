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

#include "instr_nanos6.h"

int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));
	instr_start(rank, nranks);

	int us = 500;

	int32_t typeid = 1;
	int32_t taskid = 1;

	instr_nanos6_type_create(typeid);
	instr_nanos6_task_create_and_execute(taskid, typeid);

	instr_nanos6_sched_receive_task(); usleep(us);
	instr_nanos6_sched_assign_task(); usleep(us);
	instr_nanos6_sched_self_assign_task(); usleep(us);
	instr_nanos6_sched_hungry(); usleep(us);
	instr_nanos6_sched_fill(); usleep(us);
	instr_nanos6_sched_server_enter(); usleep(us);
	instr_nanos6_sched_server_exit(); usleep(us);
	instr_nanos6_sched_submit_enter(); usleep(us);
	instr_nanos6_sched_submit_exit(); usleep(us);
	instr_nanos6_enter_submit_task(); usleep(us);
	instr_nanos6_exit_submit_task(); usleep(us);
	instr_nanos6_block_enter(); usleep(us);
	instr_nanos6_block_exit(); usleep(us);
	instr_nanos6_waitfor_enter(); usleep(us);
	instr_nanos6_waitfor_exit(); usleep(us);
	instr_nanos6_register_accesses_enter(); usleep(us);
	instr_nanos6_register_accesses_exit(); usleep(us);
	instr_nanos6_unregister_accesses_enter(); usleep(us);
	instr_nanos6_unregister_accesses_exit(); usleep(us);
	instr_nanos6_taskwait_enter(); usleep(us);
	instr_nanos6_taskwait_exit(); usleep(us);
	instr_nanos6_spawn_function_enter(); usleep(us);
	instr_nanos6_spawn_function_exit(); usleep(us);

	instr_nanos6_task_end(taskid);

	instr_end();

	return 0;
}
