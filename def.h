#ifndef OVNI_DEF_H
#define OVNI_DEF_H

#define MAX_CPU 256
#define MAX_PROC 32
#define MAX_LOOM 4
#define TRACEDIR "ovni"

struct ovnithr {
	int cpu;
	uint64_t clockvalue;
};

struct ovniproc {
	int proc;
	int loom;
	int ncpus;
	FILE *cpustream[MAX_CPU];
	atomic_int opened[MAX_CPU];
	char dir[PATH_MAX];
	clockid_t clockid;
};

struct ovniloom {
	int nprocs;
	struct ovniproc proc[MAX_PROC];
};

struct __attribute__((__packed__)) ovnievent {
	uint64_t clock;
	uint8_t fsm;
	uint8_t event;
	int32_t data;
};

struct ovnistream {
	FILE *f;
	int cpu;
	int loaded;
	int active;
	struct ovnievent last;
};

struct ovnitrace {
	int nlooms;
	struct ovniloom loom[MAX_LOOM];
	int nstreams;
	struct ovnistream *stream;
};

#endif /* OVNI_DEF_H */
