#
# Copyright (c) 2022 Barcelona Supercomputing Center (BSC)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

ovni_test(NAME nanos6-nested-tasks)
ovni_test(NAME nanos6-nested-tasks-bad SHOULD_FAIL
  REGEX "fatal: cannot execute task 1: state is not created")
ovni_test(NAME nanos6-task-types MP)
ovni_test(NAME nanos6-blocking MP)
ovni_test(NAME nanos6-subsystems MP)
ovni_test(NAME nanos6-ss-mismatch SHOULD_FAIL
  REGEX "thread [0-9]\\+ ended with 1 extra stacked nanos6 subsystems, top=ST_NANOS6_SCHED_HUNGRY")
