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

option(ENABLE_DEBUG_LOG "Enable debug messages (very verbose)")

if(ENABLE_DEBUG_LOG)
  add_definitions(-DENABLE_DEBUG)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
    "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel Asan UBsan." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_available OUTPUT error LANGUAGES C)

# Enable IPO by default, if available
if(ipo_available)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(WARNING "IPO is not supported, expect performance penalty\n${error}")
endif()

# Extra build type for AddressSanitizer (Asan)
set(CMAKE_C_FLAGS_ASAN "${CMAKE_C_FLAGS_DEBUG} \
  -fsanitize=address \
  -fno-optimize-sibling-calls \
  -fsanitize-address-use-after-scope \
  -fno-omit-frame-pointer"
  CACHE STRING "Flags used by the C compiler during AddressSanitizer builds." FORCE)

set(CMAKE_C_FLAGS_UBSAN "${CMAKE_C_FLAGS_DEBUG} -fsanitize=undefined"
  CACHE STRING "Flags used by the C compiler during UndefinedBehaviorSanitizer builds." FORCE)

add_library(ovni SHARED
  ovni.c
  parson.c
)

target_include_directories(ovni PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ovniemu
  chan.c
  emu.c
  emu_nosv.c
  emu_openmp.c
  emu_ovni.c
  emu_tampi.c
  emu_nodes.c
  emu_kernel.c
  emu_nanos6.c
  emu_task.c
  trace.c
  ovni.c
  parson.c
  pcf.c
  prv.c
)

add_executable(ovnidump
  dump.c
  ovni.c
  trace.c
  parson.c
)

add_executable(ovni2prv
  ovni2prv.c
  ovni.c
  trace.c
  parson.c
)

add_executable(ovnisort
  sort.c
  ovni.c
  trace.c
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

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)

  if(BUILD_TESTING)
    add_subdirectory(test)
  endif()
endif()

install(TARGETS ovni LIBRARY DESTINATION lib)
install(TARGETS ovniemu ovnidump ovni2prv ovnisync ovnisort RUNTIME DESTINATION bin)
install(FILES ovni.h DESTINATION include)
install(DIRECTORY cfg/ DESTINATION share/ovni)
