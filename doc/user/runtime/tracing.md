# Tracing a new program

Read carefully this document before using libovni to instrument a new
component. There are a few rules you must follow to ensure the runtime
trace is correct.

## Trace processes and threads

- Call `ovni_version_check()` once before calling any ovni function.

- Call `ovni_proc_init()` when a new process begins the execution.

- Call `ovni_thread_init()` when a new thread begins the execution
(including the main process thread). Call `ovni_flush()` and
`ovni_thread_free()` when it finishes (in that order).

- Call `ovni_proc_fini()` when a process ends, after all threads have
finished.

You can use `ovni_ev_emit()` to record a new event. If you need more
than 16 bytes of payload, use `ovni_ev_jumbo_emit()`. See the [trace
specification](../trace_spec) for more details.

Compile and link with libovni. When you run your program, a new
directory ovni will be created in the current directory `$PWD/ovni`
which contains the execution trace.

You can change the trace directory by defining the `OVNI_TRACEDIR`
environment variable. The envar accepts a trace directory name, a
relative path to the trace directory, or its absolute path. In the
first case, the trace directory will be created in the current
directory `$PWD`.

## Rules

Follow these rules to avoid losing events:

1. No event may be emitted until the process is initialized with
`ovni_proc_init()` and the thread with `ovni_thread_init()`.

2. When a thread ends the execution, it must call `ovni_flush()` to write the
events in the buffer to disk.

3. All threads must have flushed its buffers before calling `ovni_proc_fini()`.

## Select a fast directory

During the execution of your program, a per-thread buffer is kept where the new
events are being recorded. When this buffer is full, it is written to disk and
emptied, an operation known as flush. This may take a while depending on the
underliying filesystem.

Keep in mind that the thread will be blocked until the flush ends, so if your
filesystem is slow it would interrupt the execution of your program for a long
time. It is advisable to use the fastest filesystem available (see the tmpfs(5)
and df(1) manual pages).

You can select the trace directory where the buffers will be flushed during the
execution by setting the environment variable `OVNI_TMPDIR`. The last directory
will be created if doesn't exist. In that case, as soon as a process calls
`ovni_proc_fini()`, the traces of all its threads will be moved to the final
directory at `$PWD/ovni`. Example:

	OVNI_TMPDIR=$(mktemp -u /dev/shm/ovni.XXXXXX) srun ./your-app

To test the different filesystem speeds, you can use hyperfine and dd. Take a
closer look at the max time:

```
$ hyperfine 'dd if=/dev/zero of=/gpfs/projects/bsc15/bsc15557/kk bs=2M count=10'
Benchmark 1: dd if=/dev/zero of=/gpfs/projects/bsc15/bsc15557/kk bs=2M count=10
  Time (mean ± σ):      71.7 ms ± 130.4 ms    [User: 0.8 ms, System: 10.2 ms]
  Range (min … max):    14.7 ms … 1113.2 ms    162 runs
 
  Warning: Statistical outliers were detected. Consider re-running this
  benchmark on a quiet PC without any interferences from other programs. It
  might help to use the '--warmup' or '--prepare' options.

$ hyperfine 'dd if=/dev/zero of=/tmp/kk bs=2M count=10'
Benchmark 1: dd if=/dev/zero of=/tmp/kk bs=2M count=10
  Time (mean ± σ):      56.2 ms ±   5.7 ms    [User: 0.6 ms, System: 14.8 ms]
  Range (min … max):    45.8 ms …  77.8 ms    63 runs
 
$ hyperfine 'dd if=/dev/zero of=/dev/shm/kk bs=2M count=10'
Benchmark 1: dd if=/dev/zero of=/dev/shm/kk bs=2M count=10
  Time (mean ± σ):      11.4 ms ±   0.4 ms    [User: 0.5 ms, System: 11.1 ms]
  Range (min … max):     9.7 ms …  12.5 ms    269 runs
```

