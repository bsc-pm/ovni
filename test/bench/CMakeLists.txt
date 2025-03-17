# Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

if(ENABLE_ALL_TESTS)
  message(STATUS "Enabling bench tests")
else()
  message(STATUS "Disabling bench tests as ENABLE_ALL_TEST not set")
  return()
endif()

add_subdirectory(openmp)
