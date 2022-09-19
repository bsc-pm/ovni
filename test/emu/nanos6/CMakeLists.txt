# Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

ovni_test(nested-tasks.c)
ovni_test(nested-tasks-bad.c SHOULD_FAIL
  REGEX "fatal: cannot execute task 1: state is not created")
ovni_test(task-types.c MP)
ovni_test(blocking.c MP)
ovni_test(ss-mismatch.c SHOULD_FAIL
  REGEX "thread [0-9]\\+ ended with 1 extra stacked nanos6 subsystems, top=\"Worker: Looking for work\"")
