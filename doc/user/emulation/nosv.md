# nOS-V model

The [nOS-V library][nosv] implements a user space runtime that can schedule
tasks to run in multiple CPUs. The nOS-V library is instrumented to
track the internal state of the runtime as well as emit information
about the tasks that are running.

[nosv]: https://github.com/bsc-pm/nos-v

## Task model

The nOS-V runtime is composed of tasks that can be scheduled to run in
threads. Tasks can be paused and resumed, leaving the CPUs free to
execute other tasks.

In nOS-V, parallel tasks can also be scheduled multiple times and the
same task may run concurrently in several CPUs. To model this scenario,
we introduce the concept of *body*, which maps to each execution of the
same task, with a unique body id.

![Parallel tasks](fig/parallel-tasks.svg)

A normal task only has one body, while a parallel task (created with
`TASK_FLAG_PARALLEL`) can have more than one body. Each body holds the
execution state, and can transition to different execution states
following this state diagram:

![Body model](fig/body-model.svg)

Bodies begin in the Created state and transition to Running when they
begin the execution. Bodies that can be paused (created with the flag
`BODY_FLAG_PAUSE` can transition to the Paused state.

Additionally, bodies can run multiple times if they are created with the
`BODY_FLAG_RESURRECT`, and transition from Dead to Running. This
transition is required to model the tasks that implement the taskiter in
NODES, which will be submitted multiple times for execution reusing the
same task id and body id. Every time a body runs again, the iteration
number is increased.

## Task type colors

In the Paraver timeline, the color assigned to each nOS-V task type is
computed from the task type label using a hash function; the task type
id doesn't affect in any way how the color gets assigned. This method
provides two desirable properties:

- Invariant type colors over time: the order in which task types are
  created doesn't affect their color.

- Deterministic colors among threads: task types with the same label end
  up mapped to the same color, even if they are from different threads
  located in different nodes.

For more details, see [this MR][1].

[1]: https://pm.bsc.es/gitlab/rarias/ovni/-/merge_requests/27

## Subsystem view

The subsystem view provides a simplified view on what is the nOS-V
runtime doing over time. The view follows the same rules described in
the [subsystem view of Nanos6](../nanos6/#subsystem_view).
