#include "nosv_priv.h"

struct model_spec model_nosv = {
	.name = "nosv",
	.model = 'V',
	.create  = nosv_create,
	.connect = nosv_connect,
	.event   = nosv_event,
	.probe   = nosv_probe,
	.finish  = nosv_finish,
};

int
nosv_probe(struct emu *emu)
{
	if (emu->system.nthreads == 0)
		return 1;

	return 0;
}
