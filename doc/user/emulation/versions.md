# Model versions

Track changes in emulator model versions.

!!! Note
    These versions cover the [events defined](events.md) in the emulator models.
    They are **not related to the versions of the libraries**.

## Nanos6

- nanos6 1.1.0:
    - Deprecate running nested tasks (`6Tx`) without pausing the parent task
      first (`6Tp`). For compatibility, the old behavior is still supported.
- nanos6 1.0.0: Initial version

## Nodes

- nodes 1.0.0: Initial version

## Kernel

- kernel 1.0.0: Initial version

## MPI

- mpi 1.0.0: Initial version

## Ovni

- ovni 1.0.0: Initial version

## OpenMP

- openmp 1.1.0: Initial version

## TAMPI

- tampi 1.0.0: Initial version

## nOS-V

- nosv 2.0.0
    - Add support for parallel tasks, adding a new `bodyid` argument in `VT*` events.
    - Remove support for old attach events `VH{aA}`.
- nosv 1.1.0
    - Ignore old attach events `VH{aA}`.
    - Add new API attach `VA{aA}` and detach `VA{eE}` events.
- nosv 1.0.0: Initial version.
