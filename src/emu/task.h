/* Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef OVNI_EMU_TASK_H
#define OVNI_EMU_TASK_H

#include "emu.h"

struct task *task_find(struct task *tasks, uint32_t task_id);

void task_create(struct ovni_emu *emu, struct task_info *info, uint32_t type_id, uint32_t task_id);
void task_execute(struct ovni_emu *emu, struct task_stack *stack, struct task *task);
void task_pause(struct ovni_emu *emu, struct task_stack *stack, struct task *task);
void task_resume(struct ovni_emu *emu, struct task_stack *stack, struct task *task);
void task_end(struct ovni_emu *emu, struct task_stack *stack, struct task *task);

struct task_type *task_type_find(struct task_type *types, uint32_t type_id);
void task_type_create(struct task_info *info, uint32_t type_id, const char *label);

void task_create_pcf_types(struct pcf_type *pcftype, struct task_type *types);
struct task *task_get_running(struct task_stack *stack);

#endif /* OVNI_EMU_TASK_H */
