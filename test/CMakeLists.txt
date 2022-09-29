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

include(macros.cmake)

add_subdirectory(emu)
add_subdirectory(rt)
