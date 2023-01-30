#include "pvt.h"

int
pvt_open(struct pvt *pvt, long nrows, const char *dir, const char *name)
{
	memset(pvt, 0, sizeof(struct pvt));

	if (snprintf(pvt->dir, PATH_MAX, "%s", dir) >= PATH_MAX) {
		err("snprintf failed: name too long");
		return -1;
	}

	if (snprintf(pvt->name, PATH_MAX, "%s", name) >= PATH_MAX) {
		err("snprintf failed: name too long");
		return -1;
	}

	char prvpath[PATH_MAX];
	if (snprintf(prvpath, PATH_MAX, "%s/%s.prv", dir, name) >= PATH_MAX) {
		err("snprintf failed: path too long");
		return -1;
	}
	
	if (prv_open(&pvt->prv, nrows, prvpath) != 0) {
		err("prv_open failed");
		return -1;
	}

	return 0;
}

struct prv *
pvt_get_prv(struct pvt *pvt)
{
	return &pvt->prv;
}

int
pvt_advance(struct pvt *pvt, int64_t time)
{
	return prv_advance(&pvt->prv, time);
}
