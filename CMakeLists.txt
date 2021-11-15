#
# Copyright (c) 2021 Barcelona Supercomputing Center (BSC)
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

cmake_minimum_required(VERSION 3.10)

project(OVNI LANGUAGES C)

add_compile_options(-Wall -Wextra -Wformat
  -Wmissing-prototypes -Wstrict-prototypes
  #-Wconversion -Wsign-conversion
  -Wold-style-definition -pedantic
  -Werror
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS FALSE)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_available OUTPUT error LANGUAGES C)

# Enable IPO by default, if available
if(ipo_available)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(WARNING "IPO is not supported, expect performance penalty\n${error}")
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)

  if(BUILD_TESTING)
    add_subdirectory(test)
  endif()
endif()

add_library(ovni SHARED
  ovni.c
  parson.c
)

target_include_directories(ovni PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(emu
  chan.c
  emu.c
  emu_nosv.c
  emu_openmp.c
  emu_ovni.c
  emu_tampi.c
  emu_nanos6.c
  ovni.c
  parson.c
  pcf.c
  prv.c
)

add_executable(dump
  dump.c
  ovni.c
  parson.c
)

add_executable(ovni2prv
  ovni2prv.c
  ovni.c
  parson.c
)

# Use <PackageName>_ROOT variables if available, commonly used by MPI
# installations
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

find_package(MPI REQUIRED)
add_executable(ovnisync ovnisync.c)
target_link_libraries(ovnisync m MPI::MPI_C)

install(TARGETS ovni LIBRARY)
install(TARGETS emu dump ovni2prv ovnisync RUNTIME)
install(FILES ovni.h DESTINATION include)
