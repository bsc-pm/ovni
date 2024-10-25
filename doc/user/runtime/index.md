# Introduction

To use *libovni* to instrument a program follow the next instructions
carefully or you may end up with an incomplete trace that is rejected at
emulation.

You can also generate a valid trace from your own software or hardware
directly following the [trace specification](trace_spec.md).

## Initialization

To initialize libovni follow these steps in all threads:

1. **Check the version**. Call `ovni_version_check()` once before calling any
   ovni function. It can be called multiple times from any thread, but only one
   is required.

2. **Init the process**. Call `ovni_proc_init()` to initialize the process. It
   can only be called **once per process** and it must be called before the
   thread is initialized.

3. **Init the thread**. Call `ovni_thread_init()` to initialize the thread.
   Multiple attempts to initialize the same thread are ignored with a warning.
   Must be called by all threads.

The `ovni_proc_init()` arguments are as follows:

```c
void ovni_proc_init(int app, const char *loom, int pid);
```

The `app` defines the "appid" of the program, which must be a number >0. This is
useful to run multiple processes some of which run the same "app", so you can
tell which one is which. The `loom` argument defines the
[loom](../concepts/part-model.md#loom) name and maps the process to that
loom. It must be composed of the host name, a dot and a suffix. The PID is the
one obtained by `getpid(2)`.

The `ovni_thread_init()` function only accepts one argument, the TID as returned
by `gettid(2)`.

## Setup metadata

Once the process and thread are initialized, you can begin adding metadata to
the thread stream.

1. **Require models**. Call `ovni_thread_require()` with the required model
   version before emitting events for a given model. Only required once from a
   thread in a given trace. The `ovni` model is implicitly required when calling
   `ovni_thread_init()`, so there is no need to add it again.

2. **Emit loom CPUs**. Call `ovni_add_cpu()` to register each CPU in the loom. It can
   be done from a single thread or multiple threads, in the latter the list of
   CPUs is merged.

3. **Set the rank**. If you use MPI, call `ovni_proc_set_rank()` to register the
   rank and number of ranks of the current execution. Only once per process.

When emitting the CPUs with:

```c
void ovni_add_cpu(int index, int phyid);
```

The `index` will be used to identify the CPU in the loom and goes from 0 to N -
1, where N is the number of CPUs in the loom. It must match the index that is
used in affinity events when a thread switches to another CPU. The `phyid` is
only displayed in Paraver and is usually the same as the index, but it can be
different if there are multiple looms per node.

## Start the execution

The current thread must switch to the "Running" state before any event can be
processed by the emulator. Do so by emitting a [`OHx`
event](../emulation/events.md#OHx) in the stream with the appropriate payload:

```c
static void thread_execute(int32_t cpu, int32_t ctid, uint64_t tag)
{
    struct ovni_ev ev = {0};
    ovni_ev_set_clock(&ev, ovni_clock_now());
    ovni_ev_set_mcv(&ev, "OHx");
    ovni_payload_add(&ev, (uint8_t *) &cpu, sizeof(cpu));
    ovni_payload_add(&ev, (uint8_t *) &ctid, sizeof(ctid));
    ovni_payload_add(&ev, (uint8_t *) &tag, sizeof(tag));
    ovni_ev_emit(&ev);
}
```

The `cpu` is the logical index (not the physical ID) of the loom CPU at which
this thread will begin the execution. Use -1 if it is not known. The `ctid` and
`tag` allow you to track the exact point at which a given thread was created and
by which thread but they are not relevant for the first thread, so they can be
set to -1.

## Emit events

After this point you can emit any other event from this thread. Use the
`ovni_ev_*` set of functions to create and emit events. Notice that all events
refer to the current thread that emits them.

If you need to store metadata information, use the `ovni_attr_*` set of
functions. The metadata is stored in disk by `ovni_attr_flush()` and when the
thread is freed by `ovni_thread_free()`.

Attempting to emit events or writing metadata without having a thread
initialized will cause your program to abort.

## Finishing the execution

To finalize the execution **every thread** must perform the following steps,
otherwise the trace **will be rejected**.

1. **End the current thread**. Emit a [`OHe` event](../emulation/events.md#OHe) to inform the current thread ends.
2. **Flush the buffer**. Call `ovni_flush()` to be sure all events are written
   to disk.
3. **Free the thread**. Call `ovni_thread_free()` to complete the stream and
   free the memory used by the buffer.
4. **Finish the process**. If this is the last thread, call `ovni_proc_fini()`
   to set the process state to finished.

If a thread fails to perform these steps, the complete trace will be rejected by
the emulator as it cannot guarantee it to be consistent.
