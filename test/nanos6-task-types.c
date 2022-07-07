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

	int ntasks = 100;
	int ntypes = 10;

	for(int i = 0; i < ntypes; i++)
		type_create(i + 1);

	for(int i = 0; i < ntasks; i++)
	{
		task_begin(i + 1, (i % ntypes) + 1);
		usleep(500);
		task_end(i + 1);
	}

	instr_end();

	return 0;
}

