# Nanos6 Emulation

The Nanos6 emulator generates four different Paraver views, which are
explained in this document.

## Task id

The task id view represents the id of the Nanos6 task instance that is
currently executing on each thread/cpu. This id is a monotonically
increasing identifier assigned on task creation. Lower ids correspond to
tasks created at an earlier point than higher ids.

## Task type

Every task in Nanos6 contains a task type, which roughly corresponds to
the actual location in the code a task was declared. For example if a
function fn() is declared as a Nanos6 task, and it is called multiple
times in a program, every created task will have a different id, but the
same type.

In the view, each type is shown with a label declared in the source with
the label() attribute of the task. If no label was specified, one is
automatically generated for each type.

Note that in this view, event value is a hash function of the type
label, so two distinct types (tasks declared in different parts of the
code) with the same label will share event value and will hence be
indistinguishable.

## MPI Rank

Represents the current MPI rank for the currently running task in a
thread or cpu.

## Subsystem view

The subsystem view attempts to provide a general overview of what the
runtime is doing at any point in time. This view is more complex to
understand than the others but is generally the most useful to
understand what is happening and debug problems related with the
runtime.

The view shows the state of the runtime for each thread (and for each
CPU, the state of the running thread in that CPU).

The state is computed by the following method: the runtime code is
completely divided into sections of code (machine instructions) S1, S2,
..., SN, which are instrumented (an event is emitted when entering and
exiting each section), and one common section of code which is shared
across the subsystems, U, of no interest. We also assume any other code
not belonging to the runtime to be in the U section.

Every instruction of the runtime belongs to *exactly one section*.

To determine the state of a thread, we look into the stack to see what
is the top-most instrumented section.

At any given point in time, a thread may be executing code with a stack
that spawns multiple sections, for example \[ S1, U, S2, S3, U \] (the
last is on top). The subsystem view selects the last subsystem section
from the stack ignoring the common section U, and presents that section
as the current state of the execution, in this case the S3.

Additionally, the runtime sections are grouped together in systems,
which form a group of closely related functions. A complete set of
states for the subsystem view is listed below. The system is listed
first and then the subsystem:

- **No subsystem**: There is no instrumented section in the stack of the
thread.

The **Scheduler** system groups the actions that relate to the queueing
and dequeueing of ready tasks. The subsystems are:

- **Scheduler: Waiting for tasks**: Actively waiting for tasks inside the
scheduler subsystem, registered but not holding the scheduler lock

- **Scheduler: Serving tasks**: Inside the scheduler lock, serving tasks
to other threads

- **Scheduler: Adding ready tasks**: Adding tasks to the scheduler queues,
but outside of the scheduler lock.

The **Task** system contains the code that controls the lifecycle of
tasks.

- **Task: Running**: Executing the body of the task (user defined code).

- **Task: Spawning function**: Registering a new spawn function
(programmatically created task)

- **Task: Creating**: Creating a new task, through `nanos6_create_task`

- **Task: Submitting**: Submitting a recently created task, through
`nanos6_submit_task`

The **Dependency** group only contains the dependency code:

- **Dependency: Registering**: Registering a task's dependencies

- **Dependency: Unregistering**: Releasing a task's dependencies because
it has ended

- **Blocking: Taskwait**: Task is blocked while inside a taskwait

- **Blocking: Blocking current task**: Task is blocked through the Nanos6
blocking API

- **Blocking: Unblocking remote task**: Unblocking a different task using
the Nanos6 blocking API

- **Blocking: Wait For**: Blocking a deadline task, which will be
re-enqueued when a certain amount of time has passed

- **Threading: Attached as external thread**: External/Leader thread
(which has registered to Nanos6) is running

