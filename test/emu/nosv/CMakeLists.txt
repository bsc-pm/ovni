# Copyright (c) 2022-2025 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

test_emu(attach.c)
test_emu(attach-old.c SHOULD_FAIL
  REGEX "unsupported nosv model version")
test_emu(nested-tasks.c)
test_emu(task-types.c MP)
test_emu(pause.c MP)
test_emu(mp-rank.c MP)
test_emu(switch-same-type.c DISABLED) # Does it make sense anymore?
test_emu(multiple-segment.c MP NPROC 4)
test_emu(task-pause-from-submit.c)
test_emu(same-subsystem.c)
test_emu(require-missing.c SHOULD_FAIL
  REGEX "model nosv not enabled for event")
test_emu(require-missing.c
  NAME flag-enable-all
  DRIVER "flag-enable-all.driver.sh"
  REGEX "all .* models are enabled")
test_emu(cannot-breakdown.c BREAKDOWN REGEX "WARN: cannot enable breakdown for nOS-V model")
test_emu(parallel-tasks.c)
test_emu(nest-to-parallel.c)
test_emu(mutex.c)
test_emu(barrier.c)
test_emu(cond.c)

test_emu(bad-nest-same-task.c SHOULD_FAIL
  REGEX "body_execute: refusing to run body(id=1,taskid=1) in Paused state, needs to resume intead")
test_emu(bad-double-execute.c SHOULD_FAIL
  REGEX "body_execute: cannot nest body(id=1,taskid=2) over body(id=1,taskid=1) which is already running")
test_emu(bad-double-execute-parallel.c SHOULD_FAIL
  REGEX "body_execute: cannot nest body(id=1,taskid=2) over body(id=1,taskid=1) which is already running")
test_emu(bad-pause-parallel.c SHOULD_FAIL
  REGEX "body_pause: body(id=1,taskid=1) is not allowed to pause")
test_emu(bad-nest-from-parallel.c SHOULD_FAIL
  REGEX "body_execute: cannot nest body(id=2,taskid=2) over body(id=1,taskid=1) which is already running")

test_emu(events-from-outside-cpu.c SHOULD_FAIL
  REGEX "current thread .* out of CPU")
