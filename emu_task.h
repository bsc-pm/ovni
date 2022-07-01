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

void task_create(uint32_t task_id, uint32_t type_id, struct task **task_map, struct task_type **type_map);
void task_execute(uint32_t task_id, struct ovni_ethread *cur_thread, struct task **task_map, struct task **thread_task_stack);
void task_pause(uint32_t task_id, struct ovni_ethread *cur_thread, struct task **task_map, struct task **thread_task_stack);
void task_resume(uint32_t task_id, struct ovni_ethread *cur_thread, struct task **task_map, struct task **thread_task_stack);
void task_end(uint32_t task_id, struct ovni_ethread *cur_thread, struct task **task_map, struct task **thread_task_stack);
void task_type_create(uint32_t typeid, const char *label, struct task_type **type_map);
void task_create_pcf_types(struct pcf_type *pcftype, struct task_type *type_map);
struct task *task_get_running(struct task *task_stack);

#endif /* OVNI_EMU_TASK_H */
