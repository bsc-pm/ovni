/*
 * Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
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

#include "instr_nosv.h"

static void
create_and_run(int32_t id, uint32_t typeid, int us)
{
	instr_nosv_task_create(id, typeid);
	instr_nosv_task_execute(id);
	usleep(us);
}

int
main(void)
{
	instr_start(0, 1);

	int ntasks = 100;
	uint32_t typeid = 1;

	instr_nosv_type_create(typeid);

	/* Create and run the tasks, one nested into another */
	for(int i = 0; i < ntasks; i++)
		create_and_run(i + 1, typeid, 500);

	/* End the tasks in the opposite order */
	for(int i = ntasks - 1; i >= 0; i--)
		instr_nosv_task_end(i + 1);

	instr_end();

	return 0;
}

