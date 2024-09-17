![Ovni logo](fig/logo2.png)

This is the documentation of ovni, the Obtuse (but Versatile) Nanoscale
Instrumentation project.

!!! Note

	Preferably write the name of the project as lowercase *ovni*
	unless the grammar rules suggest otherwise, such as when
	starting a new sentence.

The ovni project implements a fast instrumentation library that records
small events (starting at 12 bytes) during the execution of programs to
later investigate how the execution happened.

The instrumentation process is split in two stages:
[runtime](user/runtime/index.md)
tracing and [emulation](user/emulation/index.md).

During runtime, very short binary events are stored on disk which
describe what is happening. Once the execution finishes, the events are
read and processed to reproduce the execution during the emulation
process, and the final execution trace is generated.

By splitting the runtime and emulation processes we can perform
expensive computations during the trace generation without disturbing
the runtime process.

Each event belongs to a model, which has a direct mapping to a target
library or program. Each model is independent of other models, and they
can be instrumented concurrently.

## Quick start

To start a trace follow these steps:

- First enable the instrumentation of those libraries or programs that
  you are interested in (see their documentation).
- Then simply run the program normally. You will see a new `ovni`
  directory containing the runtime trace.
- Finally run the `ovniemu ovni` command to generate the Paraver traces.
- Use the command `wxparaver ovni/cpu.prv` to load the CPU trace.
- Load the configurations from the `ovni/cfg/` directory that you are
  interested in, to open a timeline view.
