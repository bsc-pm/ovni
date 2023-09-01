# Copyright (c) 2023 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

test_emu(func.c MP)
test_emu(func-mismatch.c SHOULD_FAIL
  REGEX "thread [0-9]\\+ ended with 1 stacked mpi functions")
test_emu(func-nested.c SHOULD_FAIL
  REGEX "same value as last_value")
