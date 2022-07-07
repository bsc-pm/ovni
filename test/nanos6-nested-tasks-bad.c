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
	instr_start(0, 1);

	/* Create two nested tasks with the same task_id: this should
	 * fail */
	task_begin(1, 500);
	task_begin(1, 500);

	instr_end();

	return 0;
}

