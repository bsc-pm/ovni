# Nanos6 model

The Nanos6 runtime library implements the OmpSs-2 tasking model, which
schedules the execution of tasks with dependencies. For more information
see the [OmpSs-2 website][oss] and the [Nanos6 repository][nanos6].

[oss]: https://pm.bsc.es/ompss-2
[nanos6]: https://github.com/bsc-pm/nanos6

The library is instrumented to track the execution of tasks and also the
execution path inside the runtime library to identify what is happening.
This information is typically used by both the users and the developers
of the Nanos6 library to analyze problems and unwanted behaviors.
Towards this goal, four different Paraver views are generated, which are
explained below.

The state of each task is modelled in a simple finite state machine,
which identifies the main state changes of the task. The task is set to
the *Running* state only when is executing the body of the task,
consisting of user defined code. The states can be observed in the
following diagram:

![Nanos6 task states](fig/nanos6-task-model.svg)

## Task ID view

The task ID view represents the ID of the Nanos6 task instance that is
currently executing on each thread. This ID is a monotonically
increasing identifier assigned on task creation. Lower IDs correspond to
tasks created at an earlier point than higher IDs.

## Task type view

Every task in Nanos6 contains a task type, which roughly corresponds to
the actual location in the code a task was declared. For example if a
function is declared as a Nanos6 task, and it is called multiple times
in a program, every created task will have a different ID, but the same
type.

In the view, each type is shown with a label declared in the source with
the label attribute of the task. If no label was specified, one is
automatically generated for each type.

Note that in this view, the numeric event value is a hash function of
the type label, so two distinct types (tasks declared in different parts
of the code) with the same label will share the event value and have the
same color.

## MPI rank view

This view shows the numeric MPI rank of the process running the current
task. It is only shown when the task is in the running state. This view
is specially useful to identify task in a distributed workload, which
spans several nodes.

## Subsystem view

The subsystem view attempts to provide a general overview of what Nanos6
is doing at any point in time. This view is more complex to understand
than the others but is generally the most useful to understand what is
happening and debug problems related with Nanos6 itself.

The view shows the state of the runtime for each thread (and for each
CPU, the state of the running thread in that CPU).

The state is computed by the following method: the runtime code is
completely divided into sections of code (machine instructions) $`S_1,
S_2, \ldots, S_N`$, which are instrumented (an event is emitted when entering
and exiting each section), and one common section of code which is
shared across the subsystems, $`U`$, of no interest. We also assume any
other code not belonging to the runtime belongs to the $`U`$ section.

!!! remark

     Every instruction of the runtime belongs to *exactly one section*.

To determine the state of a thread, we look into the stack to see what
is the top-most instrumented section.

At any given point in time, a thread may be executing code with a stack
that spawns multiple sections, for example $`[ S_1, U, S_2, S_3, U ]`$
(the last is current stack frame). The subsystem view selects the last
subsystem section from the stack ignoring the common section $`U`$, and
presents that section as the current state of the execution, in this
case the section $`S_3`$.

Additionally, the runtime sections $`S_i`$ are grouped together in
subsystems, which form a closely related group of functions. The
complete list of subsystems and sections is shown below.

When there is no instrumented section in the thread stack, the state is
set to **No subsystem**.

Task subsystem
: The **Task** subsystem contains the code that controls the life cycle
of tasks. It contains the following sections:

- **Running**: Executing the body of the task (user defined code).

- **Spawning function**: Spawning a function as task (it will be
  submitted to the scheduler for later execution).

- **Creating**: Creating a new task, through `nanos6_create_task`

- **Submitting**: Submitting a recently created task, through
`nanos6_submit_task`

Scheduler subsystem
: The **Scheduler** system groups the actions that relate to the queueing
and dequeueing of ready tasks. It contains the following sections:

- **Waiting for tasks**: Actively waiting for tasks inside the
scheduler subsystem, registered but not holding the scheduler lock

- **Serving tasks**: Inside the scheduler lock, serving tasks
to other threads

- **Adding ready tasks**: Adding tasks to the scheduler queues,
but outside of the scheduler lock.

Dependency subsystem
: The **Dependency** system only contains the code that manages the
registration of task dependencies. It contains the following sections:

- **Registering**: Registering a task's dependencies

- **Unregistering**: Releasing a task's dependencies because
it has ended

Blocking subsystem
: The **Blocking** subsystem deals with the code stops the thread
execution. It contains the following sections:

- **Taskwait**: Task is blocked while inside a taskwait

- **Blocking current task**: Task is blocked through the Nanos6
blocking API

- **Unblocking remote task**: Unblocking a different task using
the Nanos6 blocking API

- **Wait For**: Blocking a deadline task, which will be
re-enqueued when a certain amount of time has passed
