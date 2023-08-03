# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Add `ovni_version_get()` function.
- Add the `ovniver` program to report the libovni version and commit.
- Add nOS-V API subsystem events for `nosv_create()` and `nosv_destroy()`.

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

[unreleased]: https://pm.bsc.es/gitlab/rarias/ovni/-/commits/master
[1.2.2]: https://pm.bsc.es/gitlab/rarias/ovni/-/tags/1.2.2
[1.2.1]: https://pm.bsc.es/gitlab/rarias/ovni/-/tags/1.2.1
[1.2.0]: https://pm.bsc.es/gitlab/rarias/ovni/-/tags/1.2.0
[1.1.0]: https://pm.bsc.es/gitlab/rarias/ovni/-/tags/1.1.0
[1.0.0]: https://pm.bsc.es/gitlab/rarias/ovni/-/tags/1.0.0
