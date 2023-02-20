#include "unittest.h"
#include "emu/thread.h"

/* Ensure we can load the old trace format */
static void
test_old_trace(void)
{
	struct thread th;

	OK(thread_init_begin(&th, "loom.0/proc.0/thread.1.obs"));
	if (th.tid != 1)
		die("wrong tid");

	OK(thread_init_begin(&th, "loom.0/proc.0/thread.2"));
	if (th.tid != 2)
		die("wrong tid");

	ERR(thread_init_begin(&th, "loom.0/proc.0/thread.kk"));
	ERR(thread_init_begin(&th, "loom.0/proc.0/thread."));
	ERR(thread_init_begin(&th, "loom.0/proc.0/thread"));
	ERR(thread_init_begin(&th, "thread.prv"));

	err("ok");
}

int main(void)
{
	test_old_trace();

	return 0;
}
