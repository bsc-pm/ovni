# Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

test_emu(ss-comm.c MP)
test_emu(ss-polling.c MP)
test_emu(ss-mismatch.c SHOULD_FAIL
  REGEX "thread [0-9]\\+ ended with 1 stacked tampi subsystems")
