# Copyright (c) 2025 Barcelona Supercomputing Center (BSC)
# SPDX-License-Identifier: GPL-3.0-or-later

check_c_compiler_flag("-fopenmp=libompv" OPENMPV_COMPILER_FOUND)
check_linker_flag(C "-fopenmp=libompv" OPENMPV_LINKER_FOUND)
cmake_path(GET CMAKE_C_COMPILER PARENT_PATH CMAKE_C_COMPILER_PATH)

if(NOT OPENMPV_COMPILER_FOUND OR NOT OPENMPV_LINKER_FOUND)
  if(ENABLE_ALL_TESTS)
    message(FATAL_ERROR "Compiler doesn't support -fopenmp=libompv flag, cannot enable OpenMP-V RT tests")
  else()
    message(STATUS "Compiler doesn't support -fopenmp=libompv flag, disabling OpenMP-V RT tests")
  endif()
  return()
endif()

find_package(Nosv)

if(NOT NOSV_FOUND)
  if(ENABLE_ALL_TESTS)
    message(FATAL_ERROR "nOS-V not found, cannot enable OpenMP-V RT tests")
  else()
    message(STATUS "nOS-V not found, disabling OpenMP-V RT tests")
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
endfunction()

openmp_bench_test(dummy.c NAME heat DRIVER heat.sh)
