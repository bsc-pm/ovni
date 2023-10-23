# Copyright (c) 2022-2024 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

test_emu(attach.c)
test_emu(attach-old.c)
test_emu(nested-tasks.c)
test_emu(nested-tasks-bad.c SHOULD_FAIL
  REGEX "body_execute: body 1 state must be Created but is Running")
test_emu(task-types.c MP)
test_emu(pause.c MP)
#test_emu(mp-rank.c MP)
test_emu(switch-same-type.c)
test_emu(multiple-segment.c MP NPROC 4)
test_emu(task-pause-from-submit.c)
test_emu(same-subsystem.c)
test_emu(require-missing.c SHOULD_FAIL
  REGEX "model nosv not enabled for event")
test_emu(require-missing.c
  NAME flag-enable-all
  DRIVER "flag-enable-all.driver.sh"
  REGEX "all .* models are enabled")
