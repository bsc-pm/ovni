# Channels

As the emulation progresses, information is written in the PRV trace to
record the new states. The emulator has specific mechanism to handle the
output of new states in the PRV trace via channels. A channel stores an
integer that represents an state at a given point in time and
corresponds to the value that will be observed in the Paraver timeline.

!!! Note

	In general, the emulator receives events, then performs a state
	transition and the new state (or states) are written into the
	PRV file.

There are two classes of channels: CPU and thread channels. Both CPU and
threads have the same fixed number of channels, given by the enumeration
`enum chan`.

For example the `CHAN_OVNI_STATE` of the thread stores the execution
state of the thread (running, paused ...). Whereas, the CPU channel
`CHAN_OVNI_NRTHREADS` records how many running threads a given CPU has.

The channels are used in the following way:

- In the "pre" phase, the emulator modifies the state of the emulator
  based on the new event. The channels are then updated accordingly in
  this phase, for example when a thread goes from running to paused it
  must update the `CHAN_OVNI_STATE` channel of the thread by also the
  `CHAN_OVNI_NRTHREADS` channel of the CPU.

- In the "emit" phase, the emulator calls the `chan_emit()` method on
  those channels that have been modified. Those have the dirty attribute
  set to 1.

- The optional "post" phase is used to perform some operations before
  the next event is loaded, but is not commonly used.

Then the emulator then loads the next event and repeats the process
again.

## Disabling and enabling channels

Some channels provide information that only makes sense in some
conditions. For example, the CPU channel `CHAN_OVNI_TID` tracks the TID
of the thread currently running in the CPU. When there is no thread
running or there are multiple threads running in the same CPU, this
channel cannot output valid information.

For those cases, the channels can be enabled or disabled as to only
provide information when it is necessary. When a channel is disabled, it
will emit the value stored in `badst` which by default is set to 0.

Notice that if a channel was in a given state A, and was disabled, it
must emit the new state is 0. When the channel is enabled again, it will
emit again the state A.

## Thread tracking channels

Regarding thread channels, there are two common conditions that cause
the channels to become disabled. When the thread is no longer running,
and then the thread is not active.

For those cases, the thread channels can be configured to automatically
be enabled or disabled, following the execution state of the thread. The
tracking mode specifies how the tracking must be done:

- `CHAN_TRACK_NONE`: nothing to track
- `CHAN_TRACK_RUNNING_TH`: enable the channel only if the thread is
  running
- `CHAN_TRACK_ACTIVE_TH`: enable the channel only if the thread is
  running, cooling or warming.

This mechanism removes the complexity of detecting when a thread stops
running, to update a channel of a given module. As the thread state
changes as handled by the `emu_ovni.c` module only.

## CPU tracking channels

Similarly, CPU channels can also be configured to track the execution
state of the threads. They become disabled when the tracking condition
is not met, but also copy the state of the tracking thread channel.

They share the same tracking modes, but their behavior is slightly
different:

In the case of tracking the running thread, if the CPU has more than one
thread running, the channel will always output the error state
`ST_TOO_MANY_TH`.

If is has no threads running, will be disabled and emit a 0 state by
default.

Otherwise, it will emit the same value as the running thread. If the
thread channel is disabled, it will emit a `ST_BAD` error state.

Regarding the active thread tracking mode, the CPU channels behave
similarly, but with the active threads instead of running ones.

The CPU tracking mechanism simplify the process of updating CPU
channels, as the modules don't need to worry about the execution model.
Only the channels need to be configured to follow the proper execution
state.

## Channel state modes

The channels can be updated in three ways:

1) A fixed state can be set to the channel using `chan_set()`, which
overrides the previous state.

2) The new state can be stored in a stack with `chan_push()` and
`chan_pop()`, to remember the history of the previous states. The
emitted event will be the one on the top.

3) Using a punctual event.

Setting the channel state is commonly used to track quantities such as
the number of threads running per CPU. While the stack mode is commonly
used to track functions or sections of code delimited with enter and
exit events, which can call an return to the previous state.

An example program may be instrumented like this:

	int bar() {
		instr("Xb[");
		...
		instr("Xb]");
	}

	int foo() {
		instr("Xf[");
		bar();
		instr("Xf]");
	}

Then, in the emulator, when processing the events "Xf[" and "Xf]", we
could track of the state as follows:

	int hook_pre_foo(struct ovni_chan *chan, int value) {
		switch(value) {
			case '[': chan_push(chan, 2); break;
			case ']': chan_pop(chan, 2); break;
			default: break;
		}
	}

	int hook_pre_bar(struct ovni_chan *chan, int value) {
		switch(value) {
			case '[': chan_push(chan, 1); break;
			case ']': chan_pop(chan, 1); break;
			default: break;
		}
	}

The channel will emit the following sequence of states: 0, 1, 2, 1, 0.

Notice that the `chan_pop()` function uses the same state being pop()'ed
as argument. The function checks that the stack contains the expected
state, forcing the emulator to always receive a matching pair of enter
and exit events.

## Punctual events

There are some conditions that are better mapped to events rather than
to state transitions. For those cases, the channels provide punctual
events which are emitted as a state than only has 1 ns of duration.

When a channel is configured to emit a punctual event with `chan_ev()`,
it will first output the new state at the current time minus 1 ns, then
restore the previous channel state and emit it at the current time.
