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

The emission phase is controlled by the [patch bay](patchbay.md) and runs all
the emit callbacks at once for all dirty channels.

## Duplicate values

When a channel feeds a duplicated value, it causes an error by default. The
behavior of each row when a duplicate value is found can be controlled by the
`flags` in `prv_register()`:

- `PRV_EMITDUP` will emit the duplicate values from the channel to the trace.

- `PRV_SKIPDUP` ignore any duplicate values and don't emit them.
