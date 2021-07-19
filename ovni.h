#ifndef OVNI_H
#define OVNI_H

int
ovni_init(int loom, int proc, int ncpus);

int
ovni_clock_update();

int
ovni_ev_worker(uint8_t fsm, uint8_t event, int32_t data);

void
ovni_cpu_set(int cpu);

void
ovni_stream_open(int cpu);

void
ovni_stream_close(int cpu);

#endif /* OVNI_H */
