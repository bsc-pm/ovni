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

int
main(void)
{
	int rank = atoi(getenv("OVNI_RANK"));
	int nranks = atoi(getenv("OVNI_NRANKS"));
	instr_start(rank, nranks);

	int us = 500;
	uint32_t typeid = 1;

	instr_nosv_type_create(typeid);
	instr_nosv_task_create(1, typeid);
	instr_nosv_task_execute(1);
	usleep(us);
	instr_nosv_task_pause(1);
	usleep(us);
	instr_nosv_task_resume(1);
	usleep(us);
	instr_nosv_task_end(1);

	instr_end();

	return 0;
}

