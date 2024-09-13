# Ovni model

The ovni model tracks the state of threads and CPUs and provides the
basis for other models.

## Thread model

The thread state is modelled as a finite state machine, depicted below:

![Thread states](fig/thread-model.svg){: style="width:80%"}

All threads begin in the `unknown` state and perform a transition to
running then the `execute` event (with MCV `OHx`) is received.
Notice that the runtime thread started the execution **before** emitting
the execute event, as it is not posible to emit the event at the exact
moment the thread begins the execution.

The asterisk `*` in the transitions states when the event is emitted. If
appears *before* the transition name, it happens *before* the real
event. If appears *after* then it is emitted *after* the real event.

In the case of the thread beginning the execution, the event is emitted
*after* the thread begins the execution, so the asterisk appears after.

### Cooling and warming states

In the Linux kernel, it is generally not possible to switch from a
thread A to another thread B in a single atomic operation, from
user-space. The thread A must first wake B and then put itself to sleep.
But the kernel is free to continue the execution of B instead of just
executing the last instructions of A so it can stop.

This limitation causes a time region at which more than one thread is
scheduled to the same CPU, which is a property that is generally
considered an error in parallel runtimes. To avoid this situation, the
additional states *cooling* and *warming* are introduced.

The cooling state indicates that the thread will be put to sleep and may
temporarily wake another thread in the same CPU. Similarly, the warming
state indicates that a thread is waking up, but it may be another thread
in its own CPU for a while.

These states allow the emulator to ignore the threads in the cooling and
warming states when only interested in those in the running state, but
at the same time allow the developers to identify the real transition
sequence to explain potential delays.

## CPU model

The CPU can only execute one thread at a time, and it may have multiple
threads assigned. When two or more threads are in the execution state,
the emulator cannot determine which one is really being executed, so it
will set all the channels to an error state.

!!! Caution

	Multiple threads running in the same CPU typically indicates an
	error of instrumentation or a problem in the runtime.

The emulator automatically switches the channels from one thread to
another when a thread is switched from the CPU. So the different models
don't need to worry about thread transitions. See the
[channels](../../dev/channels.md) section for more information.
