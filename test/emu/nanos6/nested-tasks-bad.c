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
	instr_start(0, 1);

	uint32_t typeid = 666;
	instr_nanos6_type_create(typeid);

	uint32_t taskid = 1;
	instr_nanos6_task_create_and_execute(taskid, typeid);

	/* Run another nested task with same id (should fail) */
	instr_nanos6_task_execute(taskid);

	instr_end();

	return 0;
}
