# Channels

As the emulation progresses, information is written in the PRV trace to record
the new states. The emulator has specific mechanism to handle the output of new
states in the PRV trace via channels. A channel stores a value that represents
an state at a given point in time and typically corresponds to the value that
will be observed in the Paraver timeline.

!!! Note

	The emulator receives input *events* and performs *state*
	transitions which are written into the Paraver trace.

Channels become dirty when a new value is written to them. No other
modifications are allowed by default until the channel is flushed by calling
`chan_flush()`. This behavior can be controlled with the `CHAN_DIRTY_WRITE`
property. By default, a duplicated value cannot be written into a channel,
unless the `CHAN_ALLOW_DUP` or `CHAN_IGNORE_DUP` properties are set.

A channel has an unique name used to identify the channel when debugging and
also to create derived channels. The name can be set following the printf style
when calling `chan_init()`.

## Types

There are two types of channels, **single** and **stack**. The type single only
stores one value which is updated by `chan_set()`. The stack type allows the
channel to record multiple values in a stack by using `chan_push()` and
`chan_pop()`. The value of the channels is the topmost in the stack (the last
pushed).

Notice that the `chan_pop()` function uses the same value being pop()'ed as
argument. The function checks that the stack contains the expected value,
forcing the emulator to always receive a matching pair of push and pop values.

After the channel initialization, the value of the channels is null. A channel
with an empty stack also returns the value null when being read.

## Data value

The data type of a channel is handled by the `value` structure. This structure
can store a null value, a 64 bit integer or a double. Any data type can be
written to a channel and multiple data types can be stored in the stack.

!!! Note

    For now only null and int64 types are allowed when the channel is connected
    to a Paraver trace.

## Properties

Channels have properties that can be set by `chan_prop_set()`. Setting the
`CHAN_DIRTY_WRITE` property to true allows a channel to modify its value while
being in the dirty state.

## Duplicate values

By default, writing the same value to a channel twice is forbidden and will
result in a error. The property `CHAN_ALLOW_DUP` allows writing the same value
to the channel. However, the property `CHAN_IGNORE_DUP` will ignore the attempt
to write the duplicated value with no error.

## Callback

A unique function can be set to each channel which will be called once a channel
becomes dirty with `chan_set_dirty_cb()`. This callback will be called before
`chan_set()`, `chan_push()` or `chan_pop()` returns. The [patch
bay](patchbay.md) uses this callback to detect when a channel is modified an run
other callbacks.
