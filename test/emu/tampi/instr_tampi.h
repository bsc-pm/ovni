/* Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef INSTR_TAMPI_H
#define INSTR_TAMPI_H

#include "instr.h"

INSTR_0ARG(instr_tampi_issue_nonblk_op_enter, "TCi")
INSTR_0ARG(instr_tampi_issue_nonblk_op_exit, "TCI")

INSTR_0ARG(instr_tampi_check_global_array_enter, "TGc")
INSTR_0ARG(instr_tampi_check_global_array_exit, "TGC")

INSTR_0ARG(instr_tampi_library_interface_enter, "TLi")
INSTR_0ARG(instr_tampi_library_interface_exit, "TLI")
INSTR_0ARG(instr_tampi_library_polling_enter, "TLp")
INSTR_0ARG(instr_tampi_library_polling_exit, "TLP")

INSTR_0ARG(instr_tampi_add_queues_enter, "TQa")
INSTR_0ARG(instr_tampi_add_queues_exit, "TQA")
INSTR_0ARG(instr_tampi_transfer_queues_enter, "TQt")
INSTR_0ARG(instr_tampi_transfer_queues_exit, "TQT")

INSTR_0ARG(instr_tampi_completed_request_enter, "TRc")
INSTR_0ARG(instr_tampi_completed_request_exit, "TRC")
INSTR_0ARG(instr_tampi_test_request_enter, "TRt")
INSTR_0ARG(instr_tampi_test_request_exit, "TRT")
INSTR_0ARG(instr_tampi_testall_requests_enter, "TRa")
INSTR_0ARG(instr_tampi_testall_requests_exit, "TRA")
INSTR_0ARG(instr_tampi_testsome_requests_enter, "TRs")
INSTR_0ARG(instr_tampi_testsome_requests_exit, "TRS")

INSTR_0ARG(instr_tampi_create_ticket_enter, "TTc")
INSTR_0ARG(instr_tampi_create_ticket_exit, "TTC")
INSTR_0ARG(instr_tampi_wait_ticket_enter, "TTw")
INSTR_0ARG(instr_tampi_wait_ticket_exit, "TTW")

#endif /* INSTR_TAMPI_H */
