# Paraver trace

Paraver traces are handled by the top level module `pvt` (Paraver trace), which
in turn uses the `prv`, `pcf` and `prf` to control the trace events, type
definitions and row names (in that order).

Traces must be initialized with the number of rows and a name by calling
`pvt_open()`.

The emulation time must be updated with `pvt_advance()` prior to emitting any
event, so they get the update clock in the trace.

## Connecting channels

A channel can be connected to each row in a trace with `prv_register()`, so the
new values of the channel get written in the trace. Only null and int64 data
values are supported for now.

Duplicated values cause an error by default, unless the channel is registered
with the flag `PRV_DUP`.

The emission phase is controlled by the [patch bay](../patchbay) and runs all
the emit callbacks at once for all dirty channels.
