# Model

The emulator is designed to support multiple models simultaneously. Each model
must be implemented by following the `model_spec` interface, which defines the
functions that will be called by the emulation when that model is enabled.

Each model must have a unique character that identifies the model, and will be
used to filter the events in the MCV (model, category, value) tuple of the
input event.

Unless otherwise stated, all model functions return 0 on success or -1 on error.

## Probe

The probe function determines if the model should be enabled or not, based on
the information read from the metadata of the traces. No events are available
yet. Returns -1 on error, 0 if the model must be enabled or 1 if not.

If the model is not enabled, no other function will be called.

## Create

The create function is called for each enabled model to allow them to allocate
all the required structures to perform the emulation using the
[extend](extend.md) mechanism. All the required channels must be created and
registered in the patch bay in this function, so other models can found them in
the next stage.

## Connect

In the connect function, the channels and multiplexers are connected, as all the
channels of all models have already been registered in the patch bay. The
channels must also be connected to the output traces to record the states.

## Event

The event function is called only if the processed input event matches the model
character. The function must return 0 on success or -1 on error. If an error
is returned, the emulator will print some information about the current event and
proceed to call the last stage so the traces can be closed and flushed to disk
before stopping the emulation process.

## Finish

This function is called when there are no more events to be processed or when
the emulator has encountered a problem processing on event and needs to abort
the emulation process. The output traces must be closed to write all the buffers
into disk. Additional allocated structures may now be freed.
