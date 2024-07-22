# Mark API

The mark API allows you to add arbitrary events in a trace to mark regions of
interest while debugging or developing a new program or library. The events are
processed by the emulator to generate a timeline.

## Usage in runtime

Follow these steps to correctly use the API. Most problems will be detected in
emulation and cause a panic if you are not careful.

### Create a mark type

You can create up to 100 types of marks, each will be shown in its own Paraver
timeline. To create a new type, use the following call:

```c
void ovni_mark_type(int32_t type, long flags, const char *title);
```

The numeric value `type` must be a number between 0 and 99 (both included). The
`title` will be used to give a name to the Paraver timeline.

The default with flags set to zero, is to create a channel that can hold a
single value only (see [channels](../../dev/channels.md)). To use a stack of
values add the `OVNI_MARK_STACK` value to the flags.

Only one thread among all nodes needs to define a type to be available globally,
but the same type can be defined from multiple threads, as long as the same
flags and title argument are used. The idea is to avoid having to check if the
type was already defined or not.

### Define labels (optional)

The values that are written to the channel can have labels to display in the
Paraver timeline. The labels are optional, if not given the numeric value will
be shown in Paraver.

Use the following call to register a label for a value in a given type.

```c
void ovni_mark_label(int32_t type, int64_t value, const char *label);
```

A value can only have at most a single label associated. Multiple threads can
call the `ovni_mark_label()` with the same type and value as long as they use
the same label. New labels for the same type can be associated from different
threads, as long as the values are different.

All value and label pairs are combined from all threads and will be available in
the Paraver view for each type.

### Emit events

All mark channels begin with the default value *null*, which is not shown in
Paraver and will be displayed as the usual empty space. The value of the channel
can be changed over time with the following functions.

!!! warning

    The value 0 is forbidden, as it is used by Paraver to represent the
    "empty" state.

If you have used a single channel (without `OVNI_MARK_STACK`), then you must use
the following call to emit events at runtime:

```c
void ovni_mark_set(int32_t type, int64_t value);
```

It will update the value of the channel to the given `value`.

If you have used a stack channel (with `OVNI_MARK_STACK`), then you must use the
push/pop set of calls:

```c
void ovni_mark_push(int32_t type, int64_t value);
void ovni_mark_pop(int32_t type, int64_t value);
```

The value in the pop call must match the previous pushed value.

## Usage in Paraver

Each thread holds a channel for each mark type that you have defined. The
information of the mark channels is propagated to the Paraver timeline in 
Thread and CPU views.

When a thread is not *running*, the value of the mark channels is not shown in
Paraver. In the case of the CPU timeline, only the values of the running thread are
shown. If there are no running threads, nothing is shown.

Follow the next steps to create a configuration to suit your needs. You only
need to do it once, then you can save the configuration file and reuse it for
future traces.

### Filtering the type

To see a mark type, you will have to create a Paraver configuration that matches
the type that you have created. The mark `type` value gets converted into a PRV
type by adding 100 (as values from 0 to 99 are reserved).

You can use the `cpu/ovni/mark.cfg` and `thread/ovni/mark.cfg` configurations as
a starting point to create your own.

Go to "Window Properties" (the second button under "Files & Window Properties")
and then go to Filter > Events > Event type. Set Function to `=` and click the
Types value to display the `[...]` button, which will allow you to choose which
type to display.

In the "Events Selection" window, ensure that only one type is selected, and the
"Values" panel shows Function "All", to see all values for the selected type.

### Setting the title

In the "Window Properties" adjust the Name so it reflects what you are seeing.
This will be shown in the saved images, so it is good to use a proper
description.

### Configure the display method

By default, the timeline will display the values as "Code color". To switch to a
gradient or other methods, left-click in the timeline and go to "Paint As" and
select "Gradient" (or others).

You may also want to adjust the "Drawmode" which determines what happens when
there are multiple values under a given pixel. This is specially important when
you are viewing the trace with a large time range, before zooming into a given
region.

By default, the "Random not zero" mode is selected, which will select a
random value from the ones under each pixel, disregarding the occurrences of each
value. This mode will give importance to rare values, so it is usually a safe
starting point. The "Last" mode will show the last value in that pixel, which is
more or less fair, but will often hide rare values.

To change in both horizontal (Time) and in vertical (Objects) directions, go to:
left click on timeline > Drawmode > Both > Last.

### Ensure the range is good

Paraver will only display values in the timeline that are in the Semantic 
range. If you see a red triangle in the lower left corner then there are values
outside the range that are not being displayed. You can click on this button to
expand the range to cover all values in the current view.

The opposite may also happen, where the range is too big for the current values.
You can also click on the same spot (even if the triangle is not shown) to
shrink the range to cover the values in the view, or go to the Window Properties
and modify the "Semantic Minimum" and "Semantic Maximum" values manually.

### Save the configuration

Once you finish configuring the timeline, save the configuration by
left-clicking the view and then "Save > Configuration...". You can use this
configuration in future traces to avoid doing these steps again.
