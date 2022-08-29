![Ovni logo](logo2.png)

This is the documentation of ovni, the Obtuse (but Versatile) Nanoscale
Instrumentation project.

!!! Note

	Preferably write the name of the project as lowercase *ovni*
	unless the grammar rules suggest otherwise, such as starting a
	new sentence.

The instrumentation process is split in two stages: [runtime](runtime)
tracing and [emulation](emulation/).

During runtime, very simple and short events are stored on disk which
describe what is happening. Once the execution finishes, the events are
read and processed to reproduce the execution during the emulation
process, and the final execution trace is generated.

By splitting the runtime and emulation processes we can perform
expensive computations during the trace generation without disturbing
the runtime process.

Each event belongs to a model, which has a direct mapping to a target
library or program. Each model is independent of other models, and they
can be instrumented concurrently.

The events are classified by using three identifiers known as *model*,
*category* and *value* (or MCV for short).
