# Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

if(NOT LIBOMPV_FOUND)
  if(ENABLE_ALL_TESTS)
    message(FATAL_ERROR "Compiler doesn't support -fopenmp=libompv flag, cannot enable libompv bench tests")
  else()
    message(STATUS "Compiler doesn't support -fopenmp=libompv flag, disabling libompv bench tests")
  endif()
  return()
endif()

if(NOT NOSV_FOUND)
  if(ENABLE_ALL_TESTS)
    message(FATAL_ERROR "nOS-V not found, cannot enable libompv bench tests")
  else()
    message(STATUS "nOS-V not found, disabling libompv bench tests")
  endif()
  return()
endif()

message(STATUS "Enabling libompv bench tests")

function(openmp_bench_test)
  ovni_test(${ARGN} SORT)
  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "OMP_OVNI=1")
  set_property(TEST "${OVNI_TEST_NAME}" APPEND PROPERTY
    ENVIRONMENT "NOSV_CONFIG_OVERRIDE=instrumentation.version=ovni,ovni.level=3")
  set_tests_properties("${OVNI_TEST_NAME}" PROPERTIES TIMEOUT 300)
endfunction()

find_program(B6_HEAT_OMPV_PATH b6_heat_ompv REQUIRED)
find_program(B6_HEAT_OMPV_PATH b6_heat_itampi_nodes_tasks REQUIRED)

openmp_bench_test(dummy.c NAME b6_heat_ompv DRIVER b6_heat_ompv.sh)
openmp_bench_test(dummy.c NAME b6_heat_itampi_nodes_tasks DRIVER b6_heat_itampi_nodes_tasks.sh)
openmp_bench_test(dummy.c NAME b6_heat_tampi_ompv DRIVER b6_heat_tampi_ompv.sh)
