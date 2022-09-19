# Distributed traces (MPI)

The ovni trace is designed to support concurrent programs running in different
nodes in a cluster. It is often the case that the monotonic clock
(`CLOCK_MONOTONIC`) are not synchronized between machines (in general they
measure the time since boot).

To generate a coherent Paraver trace, the offsets of the clocks need to be
provided to the emulator too. To do so, run the `ovnisync` program using MPI on
the same nodes your workload will use. If you are using SLURM, you may want to
use something like:

	% srun ./application
	% srun ovnisync

!!! warning

    Beware that you cannot launch two MPI programs inside the same srun session,
	you must invoke srun twice.

By default, it will generate the `ovni/clock-offsets.txt` file, with the
relative offsets to the rank 0 of MPI. The emulator will automatically pick the
offsets when processing the trace. Use the ovnisync `-o` option to select a
different output path (see the `-c` option in ovniemu to load the file).

Here is an example table with three nodes, all units are in nanoseconds. The
standard deviation is less than 1 us:

```
rank       hostname             offset_median        offset_mean          offset_std
0          xeon01               0                    0.000000             0.000000
1          xeon04               1165382584           1165382582.900000    135.286341
2          xeon05               3118113507           3118113599.070000    180.571610
```
