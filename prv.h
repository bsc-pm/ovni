#ifndef OVNI_PRV_H
#define OVNI_PRV_H

/* All PRV event types */
enum prv_type {
	/* Rows are CPUs */
	PTC_PROC_PID      = 10,
	PTC_THREAD_TID    = 11,
	PTC_NTHREADS      = 12,
	PTC_TASK_ID       = 20,
	PTC_TASK_TYPE_ID  = 21,

	/* Rows are threads */
	PTT_THREAD_STATE  = 60,
	PTT_THREAD_TID    = 61,
};

void
prv_ev_autocpu(struct ovni_emu *emu, int type, int val);

void
prv_ev_row(struct ovni_emu *emu, int row, int type, int val);

#endif /* OVNI_PRV_H */
