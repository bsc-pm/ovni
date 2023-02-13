# Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

include_directories(
  "${CMAKE_SOURCE_DIR}/src/include"
  "${CMAKE_SOURCE_DIR}/src/emu"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/include"
)

add_library(emu STATIC
  ../common.c
  bay.c
  chan.c
  clkoff.c
  cpu.c
  emu.c
  emu_args.c
  emu_ev.c
  model.c
  models.c
  player.c
  stream.c
  trace.c
  loom.c
  metadata.c
  mux.c
  path.c
  proc.c
  pv/pcf.c
  pv/prf.c
  pv/prv.c
  pv/pvt.c
  recorder.c
  system.c
  task.c
  thread.c
  extend.c
  ovni/probe.c
  ovni/create.c
  ovni/event.c
  nanos6/probe.c
  nanos6/connect.c
  nanos6/create.c
  nanos6/event.c
  nanos6/pvt.c
  nanos6/finish.c
  nosv/probe.c
  nosv/connect.c
  nosv/create.c
  nosv/event.c
  nosv/pvt.c
  nosv/finish.c
)

add_executable(ovniemu ovniemu.c)
target_link_libraries(ovniemu emu parson-static ovni-static)

add_executable(ovnidump ovnidump.c)
target_link_libraries(ovnidump emu parson-static ovni-static)
#
#add_executable(ovnisort ovnisort.c)
#target_link_libraries(ovnisort emu trace)
#
## Use <PackageName>_ROOT variables if available, commonly used by MPI
## installations
#if(POLICY CMP0074)
#  cmake_policy(SET CMP0074 NEW)
#endif()
#
#find_package(MPI REQUIRED)
#add_executable(ovnisync ovnisync.c)
#target_link_libraries(ovnisync m MPI::MPI_C)
#
#install(TARGETS ovniemu ovnidump ovnisync ovnisort RUNTIME DESTINATION bin)
