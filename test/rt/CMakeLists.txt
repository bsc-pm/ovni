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

find_library(nosv libnosv)
find_path(NOSV_INCLUDE_DIR nosv.h)

function(nosv_test)
  ovni_test(${ARGN})
  target_link_libraries("${OVNI_TEST_NAME}" nosv)
  target_include_directories("${OVNI_TEST_NAME}"
    PUBLIC ${NOSV_INCLUDE_DIR})
  set_property(TEST "${OVNI_TEST_NAME}" APPEND
    PROPERTY
      ENVIRONMENT "NOSV_CONFIG=${OVNI_TEST_SOURCE_DIR}/rt/nosv/nosv.toml")
endfunction()

nosv_test(NAME nosv-attach SOURCE nosv/attach.c)

add_subdirectory(nanos6)
