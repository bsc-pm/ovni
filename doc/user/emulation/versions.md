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

- ovni 1.1.0
    - Add support for mark events `OM[`, `OM]` and `OM=`
- ovni 1.0.0: Initial version

## OpenMP

- openmp 1.2.0:
    - Add support for labels and task ID views
- openmp 1.1.0: Initial version

## TAMPI

- tampi 1.0.0: Initial version

## nOS-V

- nosv 2.5.0
    - Add support for non-blocking scheduler server events `VS{Nn}`.
- nosv 2.4.0
    - Add support for `nosv_cond_wait`, `nosv_cond_signal` and `nosv_cond_broadcast` events `VA{oOgGkK}`.
- nosv 2.3.0
    - Add `nosv.can_breakdown` attribute to metadata for breakdown checks.
- nosv 2.2.0
    - Add support for progress events `VP{pra}`.
- nosv 2.1.0
    - Add support for `nosv_mutex_lock`, `nosv_mutex_trylock` and `nosv_mutex_unlock` events `VA{lLtTuU}`.
    - Add support for `nosv_barrier_wait` event `VA{bB}`.
- nosv 2.0.0
    - Add support for parallel tasks, adding a new `bodyid` argument in `VT*` events.
    - Remove support for old attach events `VH{aA}`.
- nosv 1.1.0
    - Ignore old attach events `VH{aA}`.
    - Add new API attach `VA{aA}` and detach `VA{eE}` events.
- nosv 1.0.0: Initial version.
