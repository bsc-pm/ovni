# Mux

The emulator provides a mechanism to interconnect [channels](../channels) in a
similar way as an [analog
multiplexer](https://en.wikipedia.org/wiki/Multiplexer) by using the `mux`
module.

Multiplexers or muxers have one output channel, one select channel and a
variable number of input channels (including zero). Each input channel is
registered in the multiplexer by using a key with a given value. In the normal
operation of a mux, the value of the select channel is used to find an input
with a matching key. When there is no match, the output value is set to null.

Multiplexers can be configured to execute a custom select function, which will
take the value of the select channel and determine which input should be
selected. This allows a multiplexer to act as a filter too.

## Tracking

The typical use of multiplexers is to implement the tracking modes of channels.
As an example, the following diagram shows three multiplexers used to implement
the subsystem view of [Nanos6](../nanos6):

![Patch bay](fig/mux.svg)

The first mux0 uses the thread state channel to filter the value of the
subsystem channel and only pass it to the output when the thread is in the
Running state (green). Then the filtered subsystem channel is connected to an
input of a second mux (mux1) which selects the current input of the thread
running in the CPU0. The output *nanos6.cpu.subsystem.run* is then connected to
the Paraver timeline in the row corresponding to the CPU0, which shows the
subsystem of the currently running thread.

The Nanos6 subsystem channel is also connected to the mux2, which forwards the
value to the output only when the thread state is Active. The output is directly
connected to the Paraver row assigned to that thread. This channels shows the
subsystem of the thread by only when is active (not paused nor dead). You can
see that the subsystem is shown in the thread0 at the current time (red dotted
line) when in the CPU0 the subsystem has been hidden when the thread stops being
in the Running state (yellow). 

Multiplexers allow models to interact with each other in a controlled way. In
the example, the blue channel (*nanos6.thread0.subsystem*) is directly modified by
the Nanos6 model when a new event is received. While the red channels are
controled by the ovni model.  The rest of the channels are automatically updated
in the propagation phase of the [bay](../patchbay) allowing the ovni model to
modify the Nanos6 Paraver view of the subsystems.
