# NODES model

The [NODES] runtime implements the [OmpSs-2][oss] tasking model, which
schedules the execution of tasks with dependencies. It is designed to
work on top of the [nOS-V][nos-v] runtime while providing most of the
functionalities from its predecessor, [Nanos6][nanos6].

[nodes]: https://github.com/bsc-pm/nodes
[oss]: https://pm.bsc.es/ompss-2
[nanos6]: https://github.com/bsc-pm/nanos6
[nos-v]: https://github.com/bsc-pm/nos-v

The NODES library is instrumented to track the execution of tasks in
each thread and also the execution path inside the library. The
instrumentation traces are converted into Paraver views which are
described in the following sections.

!!! Important

    It is **mandatory** to enable the nOS-V instrumentation before running a
    program with the NODES instrumentation enabled.

To enable the instrumentation of NODES on 1.4 or newer versions, add
`ovni.enabled = true` to the `nodes.toml` configuration file. Older versions
used the `NODES_OVNI=1` environment variable. Follow the
[documentation][nodes-doc] for more details.

[nodes-doc]: https://github.com/bsc-pm/nodes/tree/master/docs/tracing.md

## Subsystem view

The subsystem view provides a simplified view on what NODES is doing
over time. The view follows the same rules described in the [subsystem
view of Nanos6](nanos6.md/#subsystem_view).

The complete list of states is shown below.

- **Task subsystem**: Controls the life cycle of tasks

    - **Dependencies: Registering task accesses**: Allocating and setting
      task access dependencies.

    - **Dependencies: Unregistering task accesses**: Deallocating task
      access dependencies. Can unblock other tasks.

    - **If0: Waiting for an If0 task**: Waiting for a task with the `if(0)`
      clause to finish.

    - **If0: Executing an If0 task inline**: Executing inline a task with
      the `if(0)` clause.

    - **Taskwait: Taskwait**: Waiting in a taskwait until dependencies are
      met.

    - **Add Task: Creating a task**: Allocates memory for a new task.

    - **Add Task: Submitting a task**: Places the task in the scheduler.

    - **Spawn Function: Spawning a function**: Creating a task by
      `nanos6_spawn_function()`.
