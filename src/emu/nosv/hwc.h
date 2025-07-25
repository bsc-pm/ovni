#ifndef HWC_H
#define HWC_H

#include "common.h"

struct emu;
struct chan;

/* Store each HWC channel per emu */
struct nosv_hwc_emu {
	char **name;
	size_t n;
	int64_t *values;
};

struct nosv_hwc_thread {
	struct track *track;
	struct chan *chan;
	size_t n;
};

struct nosv_hwc_cpu {
	struct track *track;
	size_t n;
};

USE_RET int hwc_create(struct emu *emu);
USE_RET int hwc_connect(struct emu *emu);
USE_RET int hwc_event(struct emu *emu);
USE_RET int hwc_finish(struct emu *emu);

#endif /* HWC_H */
