# OpenMP Model

The LLVM OpenMP Runtime is an integral component of the LLVM compiler
infrastructure that provides support for the OpenMP (Open Multi-Processing)
programming model.

OpenMP is a widely used API and set of directives for parallel programming,
allowing developers to write multi-threaded and multi-process applications more
easily.

This documentation is about an OpenMP runtime built on top of [nOS-V][nosv],
leveraging its thread management capabilities while retaining the fundamental
characteristics of the original runtime.

While the modifications introduced to the runtime may appear to be minor, it's
important to note that this enhanced version is not API compatible with the
original runtime. As a result, it is mandatory to use the clang built in the same
[LLVM Project][llvm].

This document describes all the instrumentation features included in the runtime
by both nOS-V and OpenMP to monitor task execution and the execution flow within
the runtime library to identify what is happening. This data is useful for both
users and developers of the OpenMP runtime to analyze issues and undesired
behaviors.

[llvm]: https://pm.bsc.es/llvm-ompss
[nosv]: https://gitlab.bsc.es/nos-v/nos-v

## How to Generate Execution Traces

In order to build the OpenMP runtime nOS-V must be provided by using
`PKG_CONFIG_PATH` environment variable when configuring CMake. This results in a
runtime without instrumentation. However, the user may be able to generate
execution traces by enabling nOS-V instrumentation through
`NOSV_CONFIG_OVERRIDE="instrumentation.version=ovni"`. Note that this needs a
nOS-V installation built with ovni.

Building OpenMP with instrumentation requires to pass ovni pkg-config path to
`PKG_CONFIG_PATH` with a nosv installation compiled with ovni too. The reason is
because OpenMP is dependent of nOS-V to generate complete execution traces.

By default, OpenMP will not instrument anything. To enable instrumentation the
user must execute with `OMP_OVNI=1` and `NOSV_CONFIG_OVERRIDE="instrumentation.version=ovni"`.

The following sections will describe the OpenMP execution trace views and what
information is shown there.

## nOS-V Task Type

As said in the previous sections. This OpenMP runtime is built on top of nOS-V.
So the user can explore what does the execution do there. Here we only describe
the task type view. For other views please take a look at the nOS-V chapter.

In OpenMP, every thread that is launched (main thread included) is shown in a task
type with label "openmp". In a task application, every task call will be seen with
a task type with label "file:line:col" format referring to the pragma location. This
can be changed by using the clause label(string-literal).

OpenMP task if0 will not be shown here. Take a look at the section "Limitations" for
more information. Nevertheless, the OpenMP task view shows it.

## OpenMP Subsystem

This view illustrates the activities of each thread with different states:

- **Attached**: The thread is attached.

- **Join barrier**: The thread is in the implicit barrier of the parallel region.

- **Tasking barrier**: The thread is in the additional tasking barrier trying to
  execute tasks. This event happens if executed with KMP_TASKING=1.

- **Spin wait**: The thread spin waits for a condition. Usually this event happens
  in a barrier while waiting for the other threads to reach the barrier. The thread
  also tries to execute tasks.

- **For static**: Executing a for static. The length of the event represents all the
  chunks of iterations executed by the thread. See "Limitations" section.

- **For dynamic init**: Running the initialization of an OpenMP for dynamic.

- **For dynamic chunk**: Running a chunk of iterations of an OpenMP for dynamic. To
  clarify. If a thread executes two chunks of iterations, let's say from 1 to 4 and
  from 8 to 12, two different events will be shown. See "Limitations" section.

- **Single**: Running a Single region. All threads of the parallel region will emit
  the event.

- **Release deps**: When finishing a task, trying to release dependencies. This
  event happens although the task has no dependencies.

- **Taskwait deps**: Trying to execute tasks until dependencies have been fulfilled.
  This appears typically in a task if0 with dependencies or a taskwait with deps.

- **Invoke task**: Executing a task.

- **Invoke task if0**: Executing a task if0.

- **Task alloc**: Allocating the task descriptor.

- **Task schedule**: Adding the task to the scheduler.

- **Taskwait**: Running a taskwait.

- **Taskyield**: Running a taskyield.

- **Task dup alloc**: Duplicating the task descriptor in a taskloop.

- **Check deps**: Checking if the task has pending dependencies to be fulfilled. This
  means that if all dependencies are fulfilled the task will be scheduled.

- **Taskgroup**: Running a taskgroup.

## Limitations

By the way how OpenMP is implemented. There are some instrumentation points that
violate ovni subsystem rules. This mostly happens because some directives are lowered
partially in the transformed user code, so it is not easy to wrap them into a
Single-entry single-exit (SESE) region, like we would do with a regular task invocation,
for example.

All problematic directives are described here so the user is able to understand what
is being show in the traces

- **Task if0**: The lowered user code of a task if0 is:
  ... = __kmpc_omp_task_alloc(...);
  __kmpc_omp_taskwait_deps_51(...); // If task has dependencies
  __kmpc_omp_task_begin_if0(...);
  // Call to the user code
  omp_task_entry_(...);
  __kmpc_omp_task_complete_if0(...);

  Ideally, `omp_task_entry` should be called by the runtime to ensure the SESE structure. As
  this code is generated by the compiler it is assumed that instrumenting `__kmpc_omp_task_begin_if0`
  and `__kmpc_omp_task_complete_if0` as entry/exit points is safe and equivalent.

- **For static**: The lowered user code of a for static is:
  // Parallel code
  __kmpc_for_static_init_4(...);
  for ( i = ...; i <= ...; ++i )
    ; 
  __kmpc_for_static_fini(...);

  Ideally, the for loop should be called by the runtime to ensure the SESE structure. As
  this code is generated by the compiler it is assumed that instrumenting `__kmpc_for_static_init_4`
  and `__kmpc_for_static_fini` as entry/exit points is safe and equivalent.

- **For dynamic**: The lowered user code of a for dynamic is:

  __kmpc_dispatch_init_4(...);
  while ( __kmpc_dispatch_next_4(...))
  {
    for ( i = ...; i <= ...; ++i )
      ;
  }

  Ideally, the for loop should be called by the runtime to ensure the SESE structure. As
  this code is generated by the compiler the subsystem view shows:
  1. How long it takes to run `__kmpc_dispatch_init_4` with the event **For dynamic init**
  2. How long it takes to run from the end of 1. to the first `__kmpc_dispatch_next_4`.
  with the event **For dynamic chunk**.
  3. How long it takes to run a loop iteration chunk between the last and the previous
  `__kmpc_dispatch_next_4` call with the event **For dynamic chunk**.

