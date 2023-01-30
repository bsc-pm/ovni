/* Copyright (c) 2021-2023 Barcelona Supercomputing Center (BSC)
 * SPDX-License-Identifier: GPL-3.0-or-later */

#define ENABLE_DEBUG

#include "recorder.h"

int
recorder_init(struct recorder *rec, const char *dir)
{
	memset(rec, 0, sizeof(struct recorder));

	if (snprintf(rec->dir, PATH_MAX, "%s", dir) >= PATH_MAX) {
		err("snprintf failed: path too long");
		return -1;
	}

	return 0;
}

struct pvt *
recorder_find_pvt(struct recorder *rec, const char *name)
{
	struct pvt *pvt = NULL;
	HASH_FIND_STR(rec->pvt, name, pvt);

	return pvt;
}

struct pvt *
recorder_add_pvt(struct recorder *rec, const char *name, long nrows)
{
	struct pvt *pvt = recorder_find_pvt(rec, name);
	if (pvt != NULL) {
		err("pvt %s already registered", name);
		return NULL;
	}

	pvt = calloc(1, sizeof(struct pvt));
	if (pvt == NULL) {
		err("calloc failed:");
		return NULL;
	}

	if (pvt_open(pvt, nrows, rec->dir, name) != 0) {
		err("pvt_open failed");
		return NULL;
	}

	HASH_ADD_STR(rec->pvt, name, pvt);

	return pvt;
}

int
recorder_advance(struct recorder *rec, int64_t time)
{
	for (struct pvt *pvt = rec->pvt; pvt; pvt = pvt->hh.next) {
		if (pvt_advance(pvt, time) != 0) {
			err("pvt_advance failed");
			return -1;
		}
	}

	return 0;
}
