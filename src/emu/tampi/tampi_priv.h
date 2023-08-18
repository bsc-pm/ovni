/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef TAMPI_PRIV_H
#define TAMPI_PRIV_H

#include "emu.h"
#include "model_cpu.h"
#include "model_thread.h"

/* Private enums */

enum tampi_chan {
	CH_SUBSYSTEM = 0,
	CH_MAX,
};

enum tampi_ss_values {
	ST_COMM_ISSUE_NONBLOCKING = 1,
	ST_GLOBAL_ARRAY_CHECK,
	ST_LIBRARY_INTERFACE,
	ST_LIBRARY_POLLING,
	ST_QUEUE_ADD,
	ST_QUEUE_TRANSFER,
	ST_REQUEST_COMPLETED,
	ST_REQUEST_TEST,
	ST_REQUEST_TESTALL,
	ST_REQUEST_TESTSOME,
	ST_TICKET_CREATE,
	ST_TICKET_WAIT,
};

struct tampi_thread {
	struct model_thread m;
};

struct tampi_cpu {
	struct model_cpu m;
};

int model_tampi_probe(struct emu *emu);
int model_tampi_create(struct emu *emu);
int model_tampi_connect(struct emu *emu);
int model_tampi_event(struct emu *emu);
int model_tampi_finish(struct emu *emu);

#endif /* TAMPI_PRIV_H */
