# Copyright (c) 2021-2022 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

include_directories(
  "${CMAKE_SOURCE_DIR}/src/include"
  "${CMAKE_SOURCE_DIR}/src/emu"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/include"
)

#add_library(trace STATIC trace.c)
#target_link_libraries(trace parson ovni-static)

add_library(emu STATIC
  ../common.c
  cpu.c
  loom.c
  proc.c
  thread.c
  path.c
  metadata.c
  emu.c
  system.c
  #emu_system_thread.c
  emu_args.c
  emu_stream.c
  emu_trace.c
  emu_player.c
  emu_model.c
  emu_ev.c
  chan.c
  bay.c
  mux.c
  prv.c
  clkoff.c
  model_ust.c
)

#add_library(emu STATIC
#  chan.c
#  emu.c
#  nosv.c
#  openmp.c
#  ovni.c
#  tampi.c
#  nodes.c
#  kernel.c
#  nanos6.c
#  task.c
#  pcf.c
#  prv.c
#)
#
#add_executable(ovniemu ovniemu.c)
#target_link_libraries(ovniemu emu trace)
#
#add_executable(ovnidump ovnidump.c)
#target_link_libraries(ovnidump emu trace)
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
