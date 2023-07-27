# Copyright (c) 2022-2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

test_emu(nested-tasks.c)
test_emu(nested-tasks-bad.c SHOULD_FAIL
  REGEX "cannot execute task 1: state is not created")
test_emu(task-types.c MP)
test_emu(pause.c MP)
test_emu(mp-rank.c MP)
test_emu(switch-same-type.c)
test_emu(multiple-segment.c MP NPROC 4)
test_emu(task-pause-from-submit.c)
