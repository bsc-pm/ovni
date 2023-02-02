/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PVT_H
#define PVT_H

#include "prv.h"
#include "pcf.h"
#include "prf.h"
#include "uthash.h"
#include <linux/limits.h>

struct pvt {
	char dir[PATH_MAX];
	char name[PATH_MAX]; /* Without .prv extension */
	struct prv prv;
	struct pcf pcf;
	struct prf prf;

	struct UT_hash_handle hh; /* For recorder */
};

int pvt_open(struct pvt *pvt, long nrows, const char *dir, const char *name);
struct prv *pvt_get_prv(struct pvt *pvt);
struct pcf *pvt_get_pcf(struct pvt *pvt);
struct prf *pvt_get_prf(struct pvt *pvt);
int pvt_advance(struct pvt *pvt, int64_t time);
int pvt_close(struct pvt *pvt);

#endif /* PVT_H */
