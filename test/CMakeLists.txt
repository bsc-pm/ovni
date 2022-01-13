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

set(OVNI_TEST_DIR "${CMAKE_SOURCE_DIR}/test")

add_executable(flush flush.c)
target_link_libraries(flush ovni)
add_test(NAME flush COMMAND "${OVNI_TEST_DIR}/driver.sh" flush)
set_tests_properties(flush PROPERTIES RUN_SERIAL TRUE)

add_executable(flush-overhead flush-overhead.c)
target_link_libraries(flush-overhead ovni)
add_test(NAME flush-overhead COMMAND "${OVNI_TEST_DIR}/driver.sh" flush-overhead)
set_tests_properties(flush-overhead PROPERTIES RUN_SERIAL TRUE)

add_executable(mp-simple mp-simple.c)
target_link_libraries(mp-simple ovni)
add_test(NAME mp-simple COMMAND "${OVNI_TEST_DIR}/mp-driver.sh" mp-simple)
set_tests_properties(mp-simple PROPERTIES RUN_SERIAL TRUE)

add_executable(mp-rank mp-rank.c)
target_link_libraries(mp-rank ovni)
add_test(NAME mp-rank COMMAND "${OVNI_TEST_DIR}/mp-driver.sh" mp-rank)
set_tests_properties(mp-rank PROPERTIES RUN_SERIAL TRUE)
