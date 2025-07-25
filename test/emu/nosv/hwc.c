/* Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#include "compat.h"
#include "instr.h"
#include "instr_nosv.h"

int
main(void)
{
	instr_start(0, 1);
	instr_nosv_init();

	enum hwc { PAPI_TOT_INS = 0, PAPI_TOT_CYC = 1, MAX_HWC };
	int64_t hwc[MAX_HWC] = { 0 };

	ovni_attr_set_str("nosv.hwc.0.name", "PAPI_TOT_INS");
	ovni_attr_set_str("nosv.hwc.1.name", "PAPI_TOT_CYC");

	instr_nosv_hwc(MAX_HWC, hwc);

	for (int i = 0; i < 10; i++) {
		sleep_us(100);

		/* Dummy counters */
		hwc[PAPI_TOT_INS] = 50 + (rand() % 100);
		hwc[PAPI_TOT_CYC] = 100 + (rand() % 200);

		instr_nosv_hwc(MAX_HWC, hwc);
	}

	instr_end();

	return 0;
}
