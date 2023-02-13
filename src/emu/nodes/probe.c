#include "nodes_priv.h"

struct model_spec model_nodes = {
	.name = "nodes",
	.model = 'D',
	.create  = nodes_create,
	.connect = nodes_connect,
	.event   = nodes_event,
	.probe   = nodes_probe,
	.finish  = nodes_finish,
};

int
nodes_probe(struct emu *emu)
{
	if (emu->system.nthreads == 0)
		return 1;

	return 0;
}
