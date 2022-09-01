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

#ifndef OVNI_EMU_TASK_H
#define OVNI_EMU_TASK_H

#include "emu.h"

struct task *task_find(struct task *tasks, uint32_t task_id);

void task_create(struct task_info *info, uint32_t type_id, uint32_t task_id);
void task_execute(struct task_stack *stack, struct task *task);
void task_pause(struct task_stack *stack, struct task *task);
void task_resume(struct task_stack *stack, struct task *task);
void task_end(struct task_stack *stack, struct task *task);

struct task_type *task_type_find(struct task_type *types, uint32_t type_id);
void task_type_create(struct task_info *info, uint32_t type_id, const char *label);

void task_create_pcf_types(struct pcf_type *pcftype, struct task_type *types);
struct task *task_get_running(struct task_stack *stack);

#endif /* OVNI_EMU_TASK_H */
