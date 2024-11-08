# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.11.0] - 2024-11-08

### Added

- Introduce part model.
- Support for `nosv_cond_wait`, `nosv_cond_signal` and `nosv_cond_broadcast` events VA{oOgGkK}.

### Changed

- Enable -Wconversion and -Wsign-conversion.
- Update trace format to version 3.
- The ovni.require metadata key is now mandatory.
- Store process metadata in thread metadata.
- nOS-V model version increased to 2.4.0.

### Fixed

- Fix bug in ovnisort when injecting events in a previously modified section.

## [1.10.0] - 2024-07-26

### Changed

- nOS-V model version increased to 2.3.0.
- Prevent accidental use of nOS-V traces without required events for the
  breakdown model using the `nosv.can_breakdown` attribute.
- Increase ovni model version to 1.1.0 for the mark events `OM*`.

### Added

- Add support for nOS-V progressing events VP{pra}.
- Add breakdown model for nOS-V.
- New API to manage stream metadata `ovni_attr_*()`.
- New mark API `ovni_mark_*()` to emit user-defined events.

### Fixed

- Cross-compilation for ARM 32 bits.

## [1.9.1] - 2024-05-10

### Fixed

- Fix breakdown model error that was preventing a zero value to be written in
  the PRV trace.
- Fix gcc 11.3.1 -Wstringop-overflow warning.

## [1.9.0] - 2024-04-25

### Added

- Consistency check for events emitted when the kernel has removed the thread
  from the CPU.
- The nOS-V model version is bumped to 2.1.0.
- Add support for `nosv_mutex_lock`, `nosv_mutex_trylock` and
  `nosv_mutex_unlock` events VA{lLtTuU}.
- Add support for the `nosv_barrier_event` event VA{bB}.

### Fixed

- Open Paraver windows at (100,100) instead of (0,0) to avoid them appearing
  under the menu bar in Mac OS.

## [1.8.0] - 2024-03-18

### Added

- Add new body model to support parallel tasks in nOS-V (used in taskfor).
- Add the ability to restrict transitions in the task model states.
- Add nOS-V support for parallel tasks reading the body id from the
  event payload.
- Keep a changelog of emulation model versions.

### Changed

- The task model now requires the previous task body to be paused before nesting
  another one. A compatibility flag is still available to still allow the old
  behavior.
- Nanos6 model version increased to 1.1.0.
- nOS-V model version increased to 2.0.0.

## [1.7.0] - 2024-03-12

### Added

- Add OpenMP model (`P`) at version 1.1.0 (currently it only supports subsystems
  and only works with the OpenMP-V runtime, on top of nOS-V).

### Changed

- Add support for `nosv_attach` and `nosv_detach` events VA{aAeE}.
- Ignore old nOS-V attach events VH{aA}.
- The nOS-V model version is bumped to 1.1.0.

## [1.6.0] - 2024-02-14

### Changed

- All events now must be declared for each emulation model and given a
  description including the payload arguments.

### Added

- Manual page for ovnidump(1).
- Describe events in human readable format with ovnidump.
- New `-x` option in ovnidump to show the payload in hexadecimal.

## [1.5.1] - 2023-12-20

### Fixed

- Calling `ovni_thread_isready()` after `ovni_thread_free()` now returns 0.
- Emitting events in a non-ready thread now aborts the program with a
  message rather than causing a segfault.

## [1.5.0] - 2023-12-15

### Added

- New function `ovni_thread_require()` to selectively enable emulation models
  and check their version is compatible (if not used all models will be
  enabled).
- Support for per-thread metadata
- Store the version of libovni in the metadata
- Streams are marked as finished when `ovni_thread_free()` is called. A warning
  is emitted in the emulator for those streams that are not finished properly.
- List the emulation models and versions with `ovniemu -h`
- New `-a` ovniemu option to enable all models

### Changed

- Updated process metadata to version 2 (traces generated with an older libovni
  are not compatible with the emulator).
- Emulation models now have a semantic version (X.Y.Z) instead of just a number.
- Install ovniver with the runpath set.

### Fixed

- Close stream FD on `ovni_thread_free()`.

## [1.4.1] - 2023-11-16

### Changed

- Fix emulation for level 2 or lower in nOS-V with inline tasks by
  allowing duplicates in the subsystem channel.

## [1.4.0] - 2023-11-08

### Added

- Add `OVNI_TRACEDIR` envar to change the trace directory (default is `ovni`).

### Changed

- Don't modify nOS-V subsystem state on task pause. The "Task: Running"
  state is now renamed to "Task: In body" to reflect the change.
- Use pkg-config to locate the nOS-V library and get the version. Use
  `PKG_CONFIG_LIBDIR=/path/to/nosv/install/lib/pkgconfig` to use a custom
  installation path.

## [1.3.0] - 2023-09-07

### Added

- Add `ovni_version_get()` function.
- Add the `ovniver` program to report the libovni version and commit.
- Add nOS-V API subsystem events for `nosv_create()` and `nosv_destroy()`.
- Add TAMPI model with `T` code.
- Add subsytem events and cfgs for TAMPI model.
- Add MPI model with `M` code.
- Add function events and cfgs for MPI model.

## [1.2.2] - 2022-07-26

### Added

- Add this CHANGELOG.md file to keep track of changes.

### Fixed

- Don't rely on /tmp or $TMPDIR in tests.
- Fix misleading message in version check.
- Fix error message when opening missing trace directories

## [1.2.1] - 2022-07-25

### Fixed

- Set default visibility to hidden to prevent clashes with other functions such
  as the verr() in the glibc.
- Don't hardcore destination directory names like lib, to use the ones in the
  destination host (like lib64).
- Add support in ovnisort for cases where in the sorting region there are flush
  events.
- Remove CI nix roots from the builds to allow the garbage collector to remove
  the build.
- Fix the detection of the -fompss-2 flag in cmake by setting the flag at link
  time too.
- Fix spawn task test by waiting for the task to finish before exiting.

## [1.2.0] - 2022-05-02

This version adds the initial support for the breakdown view and some fixes.

### Added

- Add the initial breakdown view showing the task type, the subsystem or the
  idle state.
- Added the sort module for channels to support the breakdown view.
- Add the -b option in ovniemu to enable the breakdown trace.
- Add support for sponge CPUs in Nanos6.
- Add the cmake option `-DUSE_MPI=OFF` to build ovni without MPI.
- Add a test for two nOS-V shared memory segments.

### Fixed

- Sort the timeline rows in MPI rank order if given.
- Skip duplicated entries when ovnisync to runs multiple times in the same node.
- Allow tasks to be re-executed to support the taskiter in the nOS-V model.
- Use the signed `int64_t` type for clock offsets, so they can be negative.
- Avoid fmemopen() due to a bug in old glibc versions.
- Fix clang format for braced list
- Fix buffer overflow in `sort_replace()` of the sort module.
- Set the size of the channel property arrays to prevent a buffer overflow.

## [1.1.0] - 2022-03-24

This version introduces the patch bay and a big redesign in the way the channels
are used along with some other changes.

### Changed

- The channels are now connected using the patch bay, where they must be
  registered using an unique name.
- The tracking modes are implemented by using a mux, and an arbitrary way of
  tracking other channels is now possible, as it is required for the breakdown
  model.
- The channels don't have visibility with other parts of the code, they interact
  with a callback only.
- The emulator code has been split into smaller modules which are independent of
  each other, so we can unit test them separately.
- The models are now implemented following the model spec functions.
- Updated the ovni and Nanos6 models
- Model data is now stored by the `extend_set`/`get` methods, so it is kept
  separate between models.
- The CPU and thread channels have been moved to the emulator while the ones
  specific to the user space tracing with libovni.so are controlled by the ovni
  model. This allows other thread/CPU tracing mechanisms (kernel events) to
  update the emulator channels while the other models are not affected.
- The trace streams are now independent of the hierarchy loom/proc/thread, and
  end with the suffix .obs, so we can add other types of traces in the future.
- Models can register any arbitrary number of channels on runtime, so we can add
  load hardware counters in one channel each.
- Channels have a user friendly name so debugging is easier
- The die() abort mechanism has been transformed into if() + return -1, so we
  can do unit testing and check the errors and also finish the PRV traces when
  the emulator encounters an error and open them in Paraver with the last
  processed event.
- The emulator can be stopped with ^C, producing a valid Paraver trace.
- Prevents leaving threads in the running state by adding a check at the end of
  emulation
- The Paraver configurations files are copied into the trace directory.

### Removed

- The TAMPI and OpenMP models have been removed as they are not maintained.
- Punctual events are not implemented for now.
- No error states, when more than one thread is running in the virtual
  CPU, no subsystem is shown in the CPU view.

## [1.0.0] - 2022-12-16

### Added

- First ovni release.

[unreleased]: https://jungle.bsc.es/git/rarias/ovni
[1.11.0]: https://github.com/rodarima/ovni/releases/tag/1.11.0
[1.10.0]: https://github.com/rodarima/ovni/releases/tag/1.10.0
[1.9.1]: https://github.com/rodarima/ovni/releases/tag/1.9.1
[1.9.0]: https://github.com/rodarima/ovni/releases/tag/1.9.0
[1.8.0]: https://github.com/rodarima/ovni/releases/tag/1.8.0
[1.7.0]: https://github.com/rodarima/ovni/releases/tag/1.7.0
[1.6.0]: https://github.com/rodarima/ovni/releases/tag/1.6.0
[1.5.1]: https://github.com/rodarima/ovni/releases/tag/1.5.1
[1.5.0]: https://github.com/rodarima/ovni/releases/tag/1.5.0
[1.4.1]: https://github.com/rodarima/ovni/releases/tag/1.4.1
[1.4.0]: https://github.com/rodarima/ovni/releases/tag/1.4.0
[1.3.0]: https://github.com/rodarima/ovni/releases/tag/1.3.0
[1.2.2]: https://github.com/rodarima/ovni/releases/tag/1.2.2
[1.2.1]: https://github.com/rodarima/ovni/releases/tag/1.2.1
[1.2.0]: https://github.com/rodarima/ovni/releases/tag/1.2.0
[1.1.0]: https://github.com/rodarima/ovni/releases/tag/1.1.0
[1.0.0]: https://github.com/rodarima/ovni/releases/tag/1.0.0
