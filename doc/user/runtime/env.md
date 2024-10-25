# Environment variables

Some environment variables can be used to adjust settings during the execution
of libovni, they all begin with the `OVNI_` prefix. Be sure that all threads of
the same node use the same environment variables.

## OVNI_TMPDIR

During the execution of your program, a per-thread buffer is kept where the new
events are being recorded. When this buffer is full, it is written to disk and
emptied, an operation known as flush. This may take a while depending on the
underlying filesystem.

Keep in mind that the thread will be blocked until the flush ends, so if your
filesystem is slow it would interrupt the execution of your program for a long
time. It is advisable to use the fastest filesystem available (see the tmpfs(5)
and df(1) manual pages).

You can select a temporary trace directory where the buffers will be flushed
during the execution by setting the environment variable `OVNI_TMPDIR`. The last
directory will be created if doesn't exist. In that case, as soon as a process
calls `ovni_proc_fini()`, the traces of all its threads will be moved to the
final directory at `$PWD/ovni`. Example:

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

## OVNI_TRACEDIR

By default, the runtime trace will be placed in the `ovni` directory, inside the
working directory. You can specify a different location to place the trace by
setting the `OVNI_TRACEDIR` environment variable. It accepts a relative or
absolute path, which will be created if it doesn't exist.
