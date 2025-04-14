# Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

set(LIBOMPV_FLAG "-fopenmp=libompv")

include(CheckCCompilerFlag)

# Add the flag at compile and link time
set(CMAKE_REQUIRED_LINK_OPTIONS "${LIBOMPV_FLAG}")
check_c_compiler_flag("${LIBOMPV_FLAG}" LIBOMPV_FOUND)

if(NOT LIBOMPV_FOUND)
  message(STATUS "Compiler doesn't support -fopenmp=libompv")
  return()
endif()

if(NOT TARGET Libompv)
  add_library(Libompv INTERFACE)
  target_compile_options(Libompv INTERFACE "${LIBOMPV_FLAG}")
  target_link_options(Libompv INTERFACE "${LIBOMPV_FLAG}")
endif()
