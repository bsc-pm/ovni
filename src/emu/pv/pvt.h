/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PVT_H
#define PVT_H

#include <limits.h>
#include <stdint.h>
#include "common.h"
#include "pcf.h"
#include "prf.h"
#include "prv.h"
#include "uthash.h"

struct pvt {
	char dir[PATH_MAX];
	char name[PATH_MAX]; /* Without .prv extension */
	struct prv prv;
	struct pcf pcf;
	struct prf prf;

	struct UT_hash_handle hh; /* For recorder */
};

USE_RET int pvt_open(struct pvt *pvt, long nrows, const char *dir, const char *name);
USE_RET struct prv *pvt_get_prv(struct pvt *pvt);
USE_RET struct pcf *pvt_get_pcf(struct pvt *pvt);
USE_RET struct prf *pvt_get_prf(struct pvt *pvt);
USE_RET int pvt_advance(struct pvt *pvt, int64_t time);
USE_RET int pvt_close(struct pvt *pvt);

#endif /* PVT_H */
