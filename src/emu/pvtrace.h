/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#ifndef PVTRACE_H
#define PVTRACE_H

#include "prv.h"
#include "pcf.h"
#include "uthash.h"
#include <stdio.h>
#include <linux/limits.h>

struct pvtrace {
	char name[PATH_MAX];
	struct prv prv;
	struct pcf_file pcf;
};

struct pvmanager {
	struct pvtrace *traces;
};

int pvmanager_init(struct pvmanager *man);
struct pvt *pvman_new(struct pvmanager *man,
		const char *path, long nrows);

struct prv *pvt_get_prv(struct pvtrace *trace);
struct pcf *pvt_get_pcf(struct pvtrace *trace);

#endif /* PRV_H */
