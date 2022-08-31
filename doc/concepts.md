# Overview

The objective of the ovni project is to provide insight into what
happened at execution of a program.

![Instrumentation process](fig/instrumentation.svg)

The key pieces of software involved are instrumented so they emit events
during the execution which allow the reconstruction of the execution
later on.

During the execution phase, the information gathered in the events is
kept very short and simple, so the overhead is kept at minimum to avoid
disturbing the execution process. Here is an example of a single event
emitted during the execution phase, informing the current thread to
finish the execution:

	00 4f 48 65 52 c0 27 b4 d3 ec 01 00

During the emulation phase, the events are read and processed in the
emulator, reconstructing the execution. State transitions are recorded
in a Paraver trace. Here is an example of the same thread ceasing the
execution:

	2:0:1:1:1:50105669:1:0

Finally, loading the trace in the Paraver program, we can generate a
timeline visualization of the state change. Here is the example for the
same state transition of the thread stopping the execution:

![Visualization](fig/visualization.png)
