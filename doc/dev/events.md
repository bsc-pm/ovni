# Emulator events

The events processed by the emulator are described in each model.
Unrecognized events will cause a panic and stop the emulator in most
cases.

The events may have additional arguments in the payload, which are also
described. To this end, a simple language was created to specify the
format of each event in a concise declaration.

Additionally, a printf-like string is declared for each event, so they
can be explained in plain English. The values of the arguments are also
substituted in the description of the event following a extended printf
format described below.

## Event format

The events are defined by a small language that defines the MCV of an event, if
is a jumbo and the arguments in the payload (if any).

The grammar of this language is as follows in [ABNF][abnf].

[abnf]: https://en.wikipedia.org/wiki/Augmented_Backus%E2%80%93Naur_form

```ABNF
event-definition = event-mcv event-type [ "(" argument-list ")" ]
event-mcv = VCHAR VCHAR VCHAR
event-type = [ '+' ]
argument-list = argument-type " " argument-name [ ", " argument-list ]
argument-name = 1*(CHAR / DIGIT)
argument-type = type-signed | type-unsigned | type-string
type-signed = "i8" | "i16" | "i32" | "i64"
type-unsigned = "u8" | "u16" | "u32" | "u64"
type-string = "str"
```

The `event-type` defines the type of event. Using the symbol `+` defines
the event as jumbo. Otherwise, if not given it is considered a normal
event.

Here are some examples:

- `OHp`: A normal event with `OHp` MCV and no payload.

- `OAs(i32 cpu)`: A normal event with `OAs` MCV that has the cpu stored in
  the payload as a 32 bits signed integer.

- `OHC(i32 cpu, u64 tag)`: A normal event with `OHC` MCV that has two
  arguments in the payload: the cpu stored as a 32 bit signed integer,
  and a tag stored as a 64 bit unsigned integer.

- `VYc+(u32 typeid, str label)`: A jumbo event with `VTc` MCV that has in the
  jumbo payload a 32 bits unsigned integer for the typeid followed by the label
  null-terminated string of variable size.

## Event description

To describe the meaning of each event, a description follows the event
declaration. This description accepts printf-like format specifiers that
are replaced with the value of the argument they refer to.

The formats are specified as follows:

```ABNF
format-specifier = "%" [ printf-format] "{" argument-name "}"
argument-name = 1*(CHAR / DIGIT)
```

Where the optional `printf-format` is any string accepted by the format
specifier of `printf()`, as defined in the manual `printf(3)`. If the
`printf-format` is not given, the default format for the argument type
is used.

Here are some examples of event descriptions of the previous events:

```c
{ "OHp", "pauses the execution" },
{ "OAs(i32 cpu)", "switches it's own affinity to the CPU %{cpu}" },
{ "OHC(i32 cpu, u64 tag)", "creates a new thread on CPU %{cpu} with tag %#llx{tag}" },
{ "VYc+(u32 typeid, str label)", "creates task type %{typeid} with label \"%{label}\"" },
```

Which would be printed with ovnidump like:

```nohighlight
OHp  pauses the execution
OAs  switches it's own affinity to the CPU 7
OHC  creates a new thread on CPU 3 with tag 0x7f9239c6b6c0
VYc  creates task type 4 with label "block computation"
```

## Model version

When adding new events of changing the format of already existing
events, the version of the model that defines the event must be changed
accordingly to the rules of [Semantic Versions](https://semver.org).

In general, adding new events will cause a minor increase in the
version, while changing events will cause a major increase. Notice that
the emulator will reject a stream which contains events from a model
which is not compatible with the current model version.
