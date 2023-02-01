#include "nanos6_priv.h"

struct model_spec model_nanos6 = {
	.name = "nanos6",
	.model = '6',
	.create  = nanos6_create,
	.connect = nanos6_connect,
	.event   = nanos6_event,
	.probe   = nanos6_probe,
};

int
nanos6_probe(struct emu *emu)
{
	if (emu->system.nthreads == 0)
		return 1;

	return 0;
}
