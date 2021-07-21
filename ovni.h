#ifndef OVNI_H
#define OVNI_H

int
ovni_proc_init(int loom, int proc);

int
ovni_thread_init(pid_t tid);

int
ovni_thread_isready();

int
ovni_clock_update();

int
ovni_thread_ev(uint8_t fsm, uint8_t event, uint16_t a, uint16_t b);

#endif /* OVNI_H */
