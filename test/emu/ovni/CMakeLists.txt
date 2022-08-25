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

# Only run performance sensitive tests on Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  ovni_test(flush-overhead.c)
endif()

ovni_test(flush.c)
ovni_test(mp-simple.c MP)
ovni_test(mp-rank.c MP)
