# Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(ovni SHARED
  ovni.c
  parson.c
)

target_include_directories(ovni PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

configure_file("config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h" )
include_directories(${CMAKE_CURRENT_BINARY_DIR})

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

install(TARGETS ovni LIBRARY DESTINATION lib)
install(TARGETS ovniemu ovnidump ovnisync ovnisort RUNTIME DESTINATION bin)
install(FILES ovni.h DESTINATION include)
