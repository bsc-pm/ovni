# Copyright (c) 2022-2024 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

test_emu(nested-tasks.c)
test_emu(nested-tasks-bad.c SHOULD_FAIL
  REGEX "body_execute: body(id=1,taskid=1) state must be Created but is Running")
test_emu(task-types.c MP)
test_emu(blocking.c MP)
test_emu(if0-pause-parent.c)
test_emu(ss-mismatch.c SHOULD_FAIL
  REGEX "thread [0-9]\\+ ended with 1 stacked nanos6 subsystems")
test_emu(delayed-connect-ss.c)
test_emu(switch-same-type.c)
test_emu(sponge.c)
test_emu(sponge-breakdown.c BREAKDOWN)
test_emu(breakdown-no-black.c BREAKDOWN)
test_emu(breakdown-write-zero.c MP BREAKDOWN)
test_emu(rerun-task-bad.c SHOULD_FAIL
  REGEX "body_execute: body(id=1,taskid=1) is not allowed to run again")
