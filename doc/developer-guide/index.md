# Developer guide

To contribute to the ovni project, please read carefully this guide and
follow the guidelines to have your contribution accepted in the project.

## First principle: keep things simple

The most important principle in the emulator is to keep things simple.
It is extremely important to avoid any complexities, including
sacrificing the performance if necessary to keep the design simple.

## Use a formal model

Design a model of the library or runtime that you want to instrument and
all posible state transitions to be checked by the emulator. Detect
as many error conditions as posible. Otherwise, it becomes very hard to
analyze millions of state transitions by the naked eye to determine if
something went wrong.

## Emit redundant information

At runtime, emit redundant information in the events, unless they affect
the performance very badly. This information may include the ID of the
current task or any other identification, which the emulator already
knows. This redundant information is the only bit of data the emulator
can use to perform consistency checks and ensure the emulated state
follows the runtime state.

The extra information also allows a human to read the binary trace and
understand what is happening without the need to keep track of the
emulation state all the time.

## Document your model

Explain your model and the concepts behind it, so a user can understand
how to interpret the information provided by the timeline views.

The emulator only tries to represent the execution, but is never
perfect. Sometimes it is difficult or impossible to instrument what
happened at runtime, so an approximation is the only posible approach.
Explain in the documentation how the model approximates the real 
execution so the user can determine what information is only an
approximation.

## No dependencies

Don't pull any extra dependencies. If you need them, you are probably
violating the first principle (keeping things simple). Alternatively,
place a portable implementation in ovni, so we don't need to depend on
any other library when running the emulator.

## Testing

The ovni project has two types of tests: manual tests, which
artificially create the events in the trace, and only test the emulation
part. And runtime tests, which run the complete program with the
instrumentation enabled, generate a trace and then run the emulator.

When adding support for a new library or model, ensure you add enough
tests to cover the most important problems. Specially those hard to
detect without the current runtime state. See the `test/` directory for
examples of other models.
