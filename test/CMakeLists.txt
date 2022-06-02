#
# Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
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

set(OVNI_TEST_SOURCE_DIR "${CMAKE_SOURCE_DIR}/test")
set(OVNI_TEST_BUILD_DIR "${CMAKE_BINARY_DIR}/test")

macro(ovni_test name driver)
  add_executable("${name}" "${name}.c")
  target_link_libraries("${name}" ovni)

  add_test(NAME "${name}"
    COMMAND "${OVNI_TEST_SOURCE_DIR}/${driver}" "${name}"
    WORKING_DIRECTORY "${OVNI_TEST_BUILD_DIR}")

  set_property(TEST "${name}" PROPERTY RUN_SERIAL TRUE)

  if("${name}" MATCHES ".*-bad")
    set_property(TEST "${name}" PROPERTY WILL_FAIL TRUE)
  endif()

endmacro()

ovni_test("flush" "driver.sh")
ovni_test("flush-overhead" "driver.sh")
ovni_test("mp-simple" "mp-driver.sh")
ovni_test("mp-rank" "mp-driver.sh")
ovni_test("nosv-nested-tasks" "driver.sh")
ovni_test("nosv-nested-tasks-bad" "driver.sh")
ovni_test("nosv-task-types" "mp-driver.sh")
