#include "emu/loom.h"
#include "emu/cpu.h"
#include "emu/proc.h"
#include "common.h"
#include "utlist.h"

char testloom[] = "loom.0";
char testproc[] = "loom.0/proc.1";

static void
test_bad_name(struct loom *loom)
{
	if (loom_init_begin(loom, "blah") == 0)
		die("loom_init_begin didn't fail");

	if (loom_init_begin(loom, "loom/blah") == 0)
		die("loom_init_begin didn't fail");

	if (loom_init_begin(loom, "loom.123/testloom") == 0)
		die("loom_init_begin didn't fail");

	if (loom_init_begin(loom, "loom.123/") == 0)
		die("loom_init_begin didn't fail");

	if (loom_init_begin(loom, "/loom.123") == 0)
		die("loom_init_begin didn't fail");

	if (loom_init_begin(loom, "./loom.123") == 0)
		die("loom_init_begin didn't fail");

	if (loom_init_begin(loom, "loom.123") != 0)
		die("loom_init_begin failed");

	err("ok");
}

static void
test_negative_cpu(struct loom *loom)
{
	if (loom_init_begin(loom, testloom) != 0)
		die("loom_init_begin failed");

	struct cpu cpu;
	cpu_init_begin(&cpu, -1);

	if (loom_add_cpu(loom, &cpu) == 0)
		die("loom_add_cpu didn't fail");

	err("ok");
}

static void
test_duplicate_cpus(struct loom *loom)
{
	if (loom_init_begin(loom, testloom) != 0)
		die("loom_init_begin failed");

	struct cpu cpu;
	cpu_init_begin(&cpu, 123);
	if (loom_add_cpu(loom, &cpu) != 0)
		die("loom_add_cpu failed");

	if (loom_add_cpu(loom, &cpu) == 0)
		die("loom_add_cpu didn't fail");

	err("ok");
}

//static void
//test_sort_cpus(struct loom *loom)
//{
//	int ncpus = 10;
//
//	if (loom_init_begin(loom, testloom) != 0)
//		die("loom_init_begin failed");
//
//	for (int i = 0; i < ncpus; i++) {
//		int phyid = 1000 - i * i;
//		struct cpu *cpu = malloc(sizeof(struct cpu));
//		if (cpu == NULL)
//			die("malloc failed:");
//
//		cpu_init(cpu, phyid);
//		if (loom_add_cpu(loom, cpu) != 0)
//			die("loom_add_cpu failed");
//	}
//
//	if (loom_init_end(loom) != 0)
//		die("loom_init_end failed");
//
//	if (loom->ncpus != (size_t) ncpus)
//		die("ncpus mismatch");
//
//	struct cpu *cpu = NULL;
//	int lastphyid = -1;
//	DL_FOREACH2(loom->scpus, cpu, lnext) {
//		int phyid = cpu_get_phyid(cpu);
//		if (lastphyid >= phyid)
//			die("unsorted scpus");
//		lastphyid = phyid;
//	}
//
//	err("ok");
//}

static void
test_duplicate_procs(struct loom *loom)
{
	if (loom_init_begin(loom, testloom) != 0)
		die("loom_init_begin failed");

	struct proc proc;
	proc_init_begin(&proc, testproc);
	if (loom_add_proc(loom, &proc) != 0)
		die("loom_add_proc failed");

	if (loom_add_proc(loom, &proc) == 0)
		die("loom_add_proc didn't fail");

	err("ok");
}

int main(void)
{
	struct loom loom;

	test_bad_name(&loom);
	test_negative_cpu(&loom);
	test_duplicate_cpus(&loom);
	//test_sort_cpus(&loom);
	test_duplicate_procs(&loom);

	return 0;
}
