# Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

ovni_test(nested-tasks.c)
ovni_test(nested-tasks-bad.c SHOULD_FAIL
  REGEX "cannot execute task 1: state is not created")
ovni_test(task-types.c MP)
ovni_test(pause.c MP)
ovni_test(mp-rank.c MP)
ovni_test(switch-same-type.c)
