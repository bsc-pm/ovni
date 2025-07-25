# Copyright (c) 2021-2025 Barcelona Supercomputing Center (BSC)
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
  body.c
  chan.c
  clkoff.c
  cpu.c
  emu.c
  emu_args.c
  emu_ev.c
  emu_stat.c
  ev_spec.c
  model.c
  model_cpu.c
  model_thread.c
  model_pvt.c
  model_evspec.c
  models.c
  player.c
  stream.c
  trace.c
  loom.c
  mux.c
  sort.c
  path.c
  proc.c
  pv/pcf.c
  pv/prf.c
  pv/prv.c
  pv/pvt.c
  pv/cfg.c
  pv/cfg_file.c
  recorder.c
  system.c
  task.c
  track.c
  thread.c
  extend.c
  value.c
  ovni/event.c
  ovni/setup.c
  ovni/mark.c
  nanos6/setup.c
  nanos6/event.c
  nanos6/breakdown.c
  nosv/breakdown.c
  nosv/event.c
  nosv/hwc.c
  nosv/setup.c
  nodes/setup.c
  nodes/event.c
  mpi/setup.c
  mpi/event.c
  tampi/setup.c
  tampi/event.c
  kernel/setup.c
  kernel/event.c
  openmp/setup.c
  openmp/event.c
  openmp/breakdown.c
)
target_link_libraries(emu ovni-static)

add_executable(ovniemu ovniemu.c)
target_link_libraries(ovniemu emu parson-static ovni-static)

add_executable(ovnidump ovnidump.c)
target_link_libraries(ovnidump emu parson-static ovni-static)

add_executable(ovnisort ovnisort.c)
target_link_libraries(ovnisort emu parson-static ovni-static)

add_executable(ovnitop ovnitop.c)
target_link_libraries(ovnitop emu parson-static ovni-static)

add_executable(ovnievents ovnievents.c)
target_link_libraries(ovnievents emu parson-static ovni-static)

# For ovniver we need to ensure we use the RUNPATH not the RPATH.
include(CheckLinkerFlag)
check_linker_flag(C "-Wl,--enable-new-dtags" SUPPORT_ENABLE_NEW_DTAGS)
if (NOT SUPPORT_ENABLE_NEW_DTAGS)
  message(FATAL_ERROR "Flag -Wl,--enable-new-dtags not supported, refusing to build ovniver")
endif()

add_executable(ovniver ovniver.c)
target_link_libraries(ovniver ovni)
set_property(TARGET ovniver
  PROPERTY INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
target_link_options(ovniver PRIVATE "-Wl,--enable-new-dtags")

set(USE_MPI ON CACHE BOOL "Use MPI (required by ovnisync)")
if(USE_MPI)
  # Use <PackageName>_ROOT variables if available, commonly used by MPI
  # installations
  if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
  endif()
  find_package(MPI REQUIRED)
  add_executable(ovnisync ovnisync.c)
  target_link_libraries(ovnisync ovni-static m MPI::MPI_C)
  install(TARGETS ovnisync)
else()
  message(STATUS "Disabling ovnisync as MPI is disabled")
endif()

install(TARGETS ovniemu ovnidump ovnisort ovnitop ovniver)
install(FILES ovnitop.1 ovnidump.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
